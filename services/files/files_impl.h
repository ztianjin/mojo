// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_FILES_IMPL_H_
#define SERVICES_FILES_FILES_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/files/files.mojom.h"

namespace mojo {

class ApplicationConnection;

namespace files {

class FilesImpl : public Files {
 public:
  FilesImpl(ApplicationConnection* connection, InterfaceRequest<Files> request);
  ~FilesImpl() override;

  // |Files| implementation:
  void OpenFileSystem(FileSystem file_system,
                      InterfaceRequest<Directory> directory,
                      const Callback<void(Error)>& callback) override;

 private:
  StrongBinding<Files> binding_;

  DISALLOW_COPY_AND_ASSIGN(FilesImpl);
};

}  // namespace files
}  // namespace mojo

#endif  // SERVICES_FILES_FILES_IMPL_H_