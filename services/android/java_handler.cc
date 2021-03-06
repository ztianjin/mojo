// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/android/java_handler.h"

#include "base/android/base_jni_onload.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/scoped_native_library.h"
#include "jni/JavaHandler_jni.h"
#include "mojo/android/system/base_run_loop.h"
#include "mojo/android/system/core_impl.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetApplicationContext;

namespace services {
namespace android {
JavaHandler::JavaHandler() : content_handler_factory_(this) {
}

JavaHandler::~JavaHandler() {
}

void JavaHandler::RunApplication(
    mojo::InterfaceRequest<mojo::Application> application_request,
    mojo::URLResponsePtr response) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_archive_path =
      Java_JavaHandler_getNewTempLibraryPath(env, GetApplicationContext());
  base::FilePath archive_path(
      base::android::ConvertJavaStringToUTF8(env, j_archive_path.obj()));

  mojo::common::BlockingCopyToFile(response->body.Pass(), archive_path);

  jobject context = base::android::GetApplicationContext();
  Java_JavaHandler_bootstrap(
      env, context, j_archive_path.obj(),
      application_request.PassMessagePipe().release().value());
}

void JavaHandler::Initialize(mojo::ApplicationImpl* app) {
}

bool JavaHandler::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(&content_handler_factory_);
  return true;
}

}  // namespace android
}  // namespace services

namespace {

bool RegisterJNI(JNIEnv* env) {
  if (!services::android::RegisterNativesImpl(env))
    return false;

  if (!mojo::android::RegisterCoreImpl(env))
    return false;

  if (!mojo::android::RegisterBaseRunLoop(env))
    return false;

  return true;
}

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new services::android::JavaHandler());
  return runner.Run(application_request);
}

JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));
  if (!base::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !base::android::OnJNIOnLoadInit(
          std::vector<base::android::InitCallback>())) {
    return -1;
  }

  // There cannot be two AtExit objects triggering at the same time. Remove the
  // one from LibraryLoader as ApplicationRunnerChromium also uses one.
  base::android::LibraryLoaderExitHook();

  return JNI_VERSION_1_4;
}

// This is needed because the application needs to access the application
// context.
extern "C" JNI_EXPORT void InitApplicationContext(
    const base::android::JavaRef<jobject>& context) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::InitApplicationContext(env, context);
}

