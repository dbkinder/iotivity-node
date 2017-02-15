/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __IOTIVITY_NODE_HANDLES_H__
#define __IOTIVITY_NODE_HANDLES_H__

#include "../common.h"
extern "C" {
#include <ocstack.h>
}

NAPI_METHOD(JSHandle_constructor);

template <class jsType, typename T>
class JSHandle {
 public:
  T data;
  napi_ref callback;
  napi_ref self;

  std::string Init(napi_env env, napi_value _callback, napi_value _self) {
    NAPI_CALL_RETURN(napi_create_reference(env, _callback, 1, &callback));
    NAPI_CALL_RETURN(napi_create_reference(env, _self, 1, &self));
    return std::string();
  }

  static std::string New(napi_env env, napi_value *jsValue, jsType **cData) {
    napi_value theConstructor;
    HELPER_CALL_RETURN(InitClass(env, &theConstructor));
    NAPI_CALL_RETURN(
        napi_new_instance(env, theConstructor, 0, nullptr, jsValue));
    auto nativeData = std::unique_ptr<jsType>(new jsType);
    nativeData->self = nullptr;
    nativeData->callback = nullptr;
    NAPI_CALL_RETURN(
        napi_wrap(env, *jsValue, nativeData.get(), nullptr, nullptr));
    *cData = nativeData.release();
    return std::string();
  }

  static std::string Get(napi_env env, napi_value jsValue, jsType **cData) {
    napi_valuetype theType;
    NAPI_CALL_RETURN(napi_get_type_of_value(env, jsValue, &theType));
    if (theType != napi_object) {
      return LOCAL_MESSAGE("Not an object");
    }
    napi_value jsConstructor;
    HELPER_CALL_RETURN(InitClass(env, &jsConstructor));
    bool isInstanceOf;
    NAPI_CALL_RETURN(
        napi_instanceof(env, jsValue, jsConstructor, &isInstanceOf));
    if (!isInstanceOf) {
      return LOCAL_MESSAGE("Not an object of type OCDoHandle");
    }
    void *nativeDataRaw;
    NAPI_CALL_RETURN(napi_unwrap(env, jsValue, &nativeDataRaw));
    *cData = (jsType *)nativeDataRaw;
    return std::string();
  }

  static std::string Destroy(napi_env env, jsType *cData) {
    if (cData->callback) {
      NAPI_CALL_RETURN(napi_reference_release(env, cData->callback, nullptr));
    }
    if (cData->self) {
      NAPI_CALL_RETURN(napi_reference_release(env, cData->self, nullptr));
    }
    delete cData;
    return std::string();
  }

  static std::string Destroy(napi_env env, napi_value jsHandle) {
    jsType *cData;
    HELPER_CALL_RETURN(Get(env, jsHandle, &cData));
    HELPER_CALL_RETURN(Destroy(env, cData));
    return std::string();
  }

  static std::string InitClass(napi_env env,
                               napi_value *theConstructor = nullptr) {
    static napi_ref localConstructor = nullptr;
    if (!localConstructor) {
      napi_value constructorValue;
      NAPI_CALL_RETURN(napi_define_class(env, jsType::jsClassName(),
                                         JSHandle_constructor, nullptr, 0,
                                         nullptr, &constructorValue));
      NAPI_CALL_RETURN(
          napi_create_reference(env, constructorValue, 1, &localConstructor));
    }
    if (theConstructor) {
      NAPI_CALL_RETURN(
          napi_get_reference_value(env, localConstructor, theConstructor));
    }
    return std::string();
  }
};

class JSOCDoHandle : public JSHandle<JSOCDoHandle, OCDoHandle> {
 public:
  static const char *jsClassName() { return "OCDoHandle"; }
};

class JSOCResourceHandle
    : public JSHandle<JSOCResourceHandle, OCResourceHandle> {
 public:
  static const char *jsClassName() { return "OCResourceHandle"; }
};

std::string InitHandles(napi_env env);

/*
#include <nan.h>
#include <map>
extern "C" {
#include <ocstack.h>
}

template <class jsName, typename handleType>
class JSHandle {
  static Nan::Persistent<v8::FunctionTemplate> &theTemplate() {
    static Nan::Persistent<v8::FunctionTemplate> returnValue;

    if (returnValue.IsEmpty()) {
      v8::Local<v8::FunctionTemplate> theTemplate =
          Nan::New<v8::FunctionTemplate>();
      theTemplate->SetClassName(
          Nan::New(jsName::jsClassName()).ToLocalChecked());
      theTemplate->InstanceTemplate()->SetInternalFieldCount(1);
      Nan::Set(Nan::GetFunction(theTemplate).ToLocalChecked(),
               Nan::New("displayName").ToLocalChecked(),
               Nan::New(jsName::jsClassName()).ToLocalChecked());
      returnValue.Reset(theTemplate);
    }
    return returnValue;
  }

 public:
  static v8::Local<v8::Object> New(handleType data) {
    v8::Local<v8::Object> returnValue =
        Nan::NewInstance(
            Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked())
            .ToLocalChecked();
    Nan::SetInternalFieldPointer(returnValue, 0, data);

    return returnValue;
  }

  // If the object is not of the expected type, or if the pointer inside the
  // object has already been removed, then we must throw an error
  static handleType Resolve(v8::Local<v8::Object> jsObject) {
    handleType returnValue = 0;

    if (Nan::New(theTemplate())->HasInstance(jsObject)) {
      returnValue = (handleType)Nan::GetInternalFieldPointer(jsObject, 0);
    }
    if (!returnValue) {
      Nan::ThrowTypeError(
          (std::string("Object is not of type ") + jsName::jsClassName())
              .c_str());
    }
    return returnValue;
  }
};

class JSOCRequestHandle : public JSHandle<JSOCRequestHandle, OCRequestHandle> {
 public:
  static const char *jsClassName() { return "OCRequestHandle"; }
};

template <typename handleType>
class CallbackInfo {
 public:
  handleType handle;
  Nan::Callback callback;
  Nan::Persistent<v8::Object> jsHandle;
  v8::Local<v8::Object> Init(v8::Local<v8::Object> _jsHandle,
                             v8::Local<v8::Function> jsCallback) {
    callback.Reset(jsCallback);
    jsHandle.Reset(_jsHandle);
    return _jsHandle;
  }
  CallbackInfo() : handle(0) {}
  virtual ~CallbackInfo() {
    if (!jsHandle.IsEmpty()) {
      v8::Local<v8::Object> theObject = Nan::New(jsHandle);
      Nan::SetInternalFieldPointer(theObject, 0, 0);
      Nan::ForceSet(theObject, Nan::New("stale").ToLocalChecked(),
                    Nan::New(true),
                    (v8::PropertyAttribute)(v8::ReadOnly | v8::DontDelete));
      jsHandle.Reset();
    }
  }
};

#define JSCALLBACKHANDLE_RESOLVE(type, info, object, ...) \
  do {                                                    \
    info = type::Resolve((object));                       \
    if (!info) {                                          \
      return __VA_ARGS__;                                 \
    }                                                     \
  } while (0)

class JSOCDoHandle : public JSHandle<JSOCDoHandle, CallbackInfo<OCDoHandle> *> {
 public:
  static const char *jsClassName() { return "OCDoHandle"; }
};

class JSOCResourceHandle
    : public JSHandle<JSOCResourceHandle, CallbackInfo<OCResourceHandle> *> {
 public:
  static const char *jsClassName() { return "OCResourceHandle"; }
  static std::map<OCResourceHandle, Nan::Persistent<v8::Object> *> handles;
};

v8::Local<v8::Array> jsArrayFromBytes(unsigned char *bytes, uint32_t length);

bool fillCArrayFromJSArray(unsigned char *bytes, uint32_t length,
                           v8::Local<v8::Array> array);
*/

#endif /* __IOTIVITY_NODE_HANDLES_H__ */
