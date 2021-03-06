// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:core';
import 'dart:typed_data';

import 'package:mojo/public/dart/application.dart';
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';
import 'package:mojom/mojo/files/file.mojom.dart' as files;
import 'package:mojom/mojo/files/types.mojom.dart' as files;
import 'package:mojom/mojo/net_address.mojom.dart';
import 'package:mojom/mojo/network_error.mojom.dart';
import 'package:mojom/mojo/network_service.mojom.dart';
import 'package:mojom/mojo/tcp_bound_socket.mojom.dart';
import 'package:mojom/mojo/tcp_connected_socket.mojom.dart';
import 'package:mojom/mojo/terminal/terminal_client.mojom.dart';

void ignoreFuture(Future f) {
  f.catchError((e) {});
}

NetAddress makeIPv4NetAddress(List<int> addr, int port) {
  var rv = new NetAddress();
  rv.family = NetAddressFamily_IPV4;
  rv.ipv4 = new NetAddressIPv4();
  rv.ipv4.addr = new List<int>.from(addr);
  rv.ipv4.port = port;
  return rv;
}

void fputs(files.File f, String s) {
  ignoreFuture(f.write((s + '\n').codeUnits, 0, files.Whence_FROM_CURRENT));
}

// Connects the terminal |File| and the socket.
// TODO(vtl):
// * Error handling: both connection/socket errors and terminal errors.
// * Relatedly, we should listen for _socketSender's peer being closed (also
//   _socket, I guess).
// * Handle the socket send pipe being full (currently, we assume it's never
//   full).
class Connector {
  final Application _application;
  final files.FileProxy _terminal;
  TcpConnectedSocketProxy _socket;
  MojoDataPipeProducer _socketSender;
  MojoDataPipeConsumer _socketReceiver;
  MojoEventStream _socketReceiverEventStream;
  final ByteData _readBuffer;
  final ByteData _writeBuffer;

  // TODO(vtl): Don't just hard-code buffer sizes.
  Connector(this._application, this._terminal)
      : _readBuffer = new ByteData(16 * 1024),
        _writeBuffer = new ByteData(16 * 1024);

  Future connect(NetAddress remote_address) async {
    var networkService = new NetworkServiceProxy.unbound();
    _application.connectToService('mojo:network_service', networkService);

    NetAddress local_address = makeIPv4NetAddress([0, 0, 0, 0], 0);
    var boundSocket = new TcpBoundSocketProxy.unbound();
    await networkService.ptr.createTcpBoundSocket(local_address, boundSocket);
    await networkService.close();

    var sendDataPipe = new MojoDataPipe();
    _socketSender = sendDataPipe.producer;
    var receiveDataPipe = new MojoDataPipe();
    _socketReceiver = receiveDataPipe.consumer;
    _socket = new TcpConnectedSocketProxy.unbound();
    await boundSocket.ptr.connect(remote_address, sendDataPipe.consumer,
        receiveDataPipe.producer, _socket);
    await boundSocket.close();

    // Set up reading from the terminal.
    _startReadingFromTerminal();

    // Set up reading from the socket.
    _socketReceiverEventStream = new MojoEventStream(_socketReceiver.handle);
    _socketReceiverEventStream.listen(_onSocketReceiverEvent);
  }

  void _startReadingFromTerminal() {
    // TODO(vtl): Handle terminal errors.
    _terminal.ptr
        .read(_writeBuffer.lengthInBytes, 0, files.Whence_FROM_CURRENT)
        .then(_onReadFromTerminal);
  }

  void _onReadFromTerminal(files.FileReadResponseParams p) {
    if (p.error != files.Error_OK) {
      // TODO(vtl): Do terminal errors.
      return;
    }

    // TODO(vtl): Temporary hack: echo, since we don't have built-in echo
    // support.
    ignoreFuture(
        _terminal.ptr.write(p.bytesRead, 0, files.Whence_FROM_CURRENT));

    // TODO(vtl): Verify that |bytesRead.length| is within the expected range.
    for (var i = 0, j = 0; i < p.bytesRead.length; i++, j++) {
      // TODO(vtl): Temporary hack: Translate \r to \n, since we don't have
      // built-in support for that.
      if (p.bytesRead[i] == 13) {
        _writeBuffer.setUint8(i, 10);
      } else {
        _writeBuffer.setUint8(i, p.bytesRead[i]);
      }
    }

    // TODO(vtl): Handle the send data pipe being full (or closed).
    _socketSender
        .write(new ByteData.view(_writeBuffer.buffer, 0, p.bytesRead.length));

    _startReadingFromTerminal();
  }

  void _onSocketReceiverEvent(List<int> event) {
    var mojoSignals = new MojoHandleSignals(event[1]);
    var shouldClose = false;
    if (mojoSignals.isReadable) {
      var numBytesRead = _socketReceiver.read(_readBuffer);
      if (_socketReceiver.status.isOk) {
        assert(numBytesRead > 0);
        _terminal.ptr.write(_readBuffer.buffer.asUint8List(0, numBytesRead), 0,
            files.Whence_FROM_CURRENT);
        _socketReceiverEventStream.enableReadEvents();
      } else {
        shouldClose = true;
      }
    } else if (mojoSignals.isPeerClosed) {
      shouldClose = true;
    } else {
      throw 'Unexpected handle event: $mojoSignals';
    }
    if (shouldClose) {
      _socketReceiverEventStream.close();
      _socketReceiverEventStream = null;
      fputs(_terminal.ptr, 'Connection closed.');
    }
  }
}

class TerminalClientImpl implements TerminalClient {
  TerminalClientStub _stub;
  Application _application;
  String _resolvedUrl;

  TerminalClientImpl(
      this._application, this._resolvedUrl, MojoMessagePipeEndpoint endpoint) {
    _stub = new TerminalClientStub.fromEndpoint(endpoint, this);
  }

  @override
  void connectToTerminal(files.FileProxy terminal) {
    var url = Uri.parse(_resolvedUrl);
    NetAddress remote_address;
    try {
      remote_address = _getNetAddressFromUrl(url);
    } catch (e) {
      fputs(terminal.ptr, 'HALP: Add a query: ?host=<host>&port=<port>\n'
          '(<host> must be "localhost" or n1.n2.n3.n4)\n\n'
          'Got query parameters:\n' + url.queryParameters.toString());
      ignoreFuture(terminal.close());
      return;
    }

    // TODO(vtl): Currently, we only do IPv4, so this should work.
    fputs(terminal.ptr, 'Connecting to: ' +
        remote_address.ipv4.addr.join('.') +
        ':' +
        remote_address.ipv4.port.toString() +
        '...');

    var connector = new Connector(_application, terminal);
    connector.connect(remote_address);
  }

  // Note: May throw all sorts of things.
  static NetAddress _getNetAddressFromUrl(Uri url) {
    var params = url.queryParameters;
    var host = params['host'];
    return makeIPv4NetAddress(
        (host == 'localhost') ? [127, 0, 0, 1] : Uri.parseIPv4Address(host),
        int.parse(params['port']));
  }
}

class NetcatApplication extends Application {
  NetcatApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(TerminalClientName,
        (endpoint) => new TerminalClientImpl(this, resolvedUrl, endpoint));
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  new NetcatApplication.fromHandle(appHandle)
    ..onError = (() {
      assert(MojoHandle.reportLeakedHandles());
    });
}
