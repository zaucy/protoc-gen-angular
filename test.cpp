string GetDeserializeMethodName(const string& mode_var) {
  return "deserializeBinary";
}

string GetSerializeMethodName(const string& mode_var) {
  return "serializeBinary";
}

string LowercaseFirstLetter(string s) {
  if (s.empty()) {
    return s;
  }
  s[0] = ::tolower(s[0]);
  return s;
}

/* Finds all message types used in all services in the file, and returns them
 * as a map of fully qualified message type name to message descriptor */
std::map<string, const Descriptor*> GetAllMessages(const FileDescriptor* file) {
  std::map<string, const Descriptor*> message_types;
  for (int service_index = 0;
       service_index < file->service_count();
       ++service_index) {
    const ServiceDescriptor* service = file->service(service_index);
    for (int method_index = 0;
         method_index < service->method_count();
         ++method_index) {
      const MethodDescriptor *method = service->method(method_index);
      const Descriptor *input_type = method->input_type();
      const Descriptor *output_type = method->output_type();
      message_types[input_type->full_name()] = input_type;
      message_types[output_type->full_name()] = output_type;
    }
  }

  return message_types;
}

void PrintMessagesDeps(Printer* printer, const FileDescriptor* file) {
  std::map<string, const Descriptor*> messages = GetAllMessages(file);
  std::map<string, string> vars;
  for (std::map<string, const Descriptor*>::iterator it = messages.begin();
       it != messages.end(); it++) {
    vars["full_name"] = it->first;
    printer->Print(
        vars,
        "goog.require('proto.$full_name$');\n");
  }
  printer->Print("\n\n\n");
}

void PrintFileHeader(Printer* printer, const std::map<string, string>& vars) {
  printer->Print(
      vars,
      "/**\n"
      " * @fileoverview gRPC Angular generated service for $package$\n"
      " * @enhanceable\n"
      " * @public\n"
      " */\n"
      "// GENERATED CODE -- DO NOT EDIT!\n\n\n");
}

void PrintServiceConstructor(Printer* printer,
                             const std::map<string, string>& vars) {
  printer->Print(
      vars,
      "/**\n"
      " * @param {string} hostname\n"
      " * @param {?Object} credentials\n"
      " * @constructor\n"
      " * @struct\n"
      " * @final\n"
      " */\n"
      "proto.$package_dot$$service_name$Client =\n"
      "    function(hostname, credentials, options) {\n"
      "  /**\n"
      "   * @private @const {!grpc.web.$mode$ClientBase} The client\n"
      "   */\n"
      "  this.client_ = new grpc.web.$mode$ClientBase(options);\n\n"
      "  /**\n"
      "   * @private @const {string} The hostname\n"
      "   */\n"
      "  this.hostname_ = hostname;\n\n"
      "  /**\n"
      "   * @private @const {?Object} The credentials to be used to connect\n"
      "   *    to the server\n"
      "   */\n"
      "  this.credentials_ = credentials;\n\n"
      "  /**\n"
      "   * @private @const {?Object} Options for the client\n"
      "   */\n"
      "  this.options_ = options;\n"
      "};\n\n\n");
}

void PrintMethodInfo(Printer* printer, std::map<string, string> vars) {
  printer->Print(
      vars,
      "/**\n"
      " * @const\n"
      " * @type {!grpc.web.AbstractClientBase.MethodInfo<\n"
      " *   !proto.$in$,\n"
      " *   !proto.$out$>}\n"
      " */\n"
      "const methodInfo_$method_name$ = "
      "new grpc.web.AbstractClientBase.MethodInfo(\n");
  printer->Indent();
  printer->Print(
      vars,
      "proto.$out$,\n"
      "/** @param {!proto.$in$} request */\n"
      "function(request) {\n");
  printer->Print(
      ("  return request." + GetSerializeMethodName(vars["mode"]) +
       "();\n").c_str());
  printer->Print("},\n");
  printer->Print(
      vars,
      ("proto.$out$." + GetDeserializeMethodName(vars["mode"]) +
       "\n").c_str());
  printer->Outdent();
  printer->Print(
      vars,
      ");\n\n\n");
}

void PrintUnaryCall(Printer* printer, std::map<string, string> vars) {
  PrintMethodInfo(printer, vars);
  printer->Print(
      vars,
      "/**\n"
      " * @param {!proto.$in$} request The\n"
      " *     request proto\n"
      " * @param {!Object<string, string>} metadata User defined\n"
      " *     call metadata\n"
      " * @param {function(?grpc.web.Error,"
      " ?proto.$out$)}\n"
      " *     callback The callback function(error, response)\n"
      " * @return {!grpc.web.ClientReadableStream<!proto.$out$>|undefined}\n"
      " *     The XHR Node Readable Stream\n"
      " */\n"
      "proto.$package_dot$$service_name$Client.prototype.$js_method_name$ =\n");
  printer->Indent();
  printer->Print(vars,
                 "  function(request, metadata, callback) {\n"
                 "return this.client_.rpcCall(this.hostname_ +\n");
  printer->Indent();
  printer->Indent();
  printer->Print(vars, "'/$package_dot$$service_name$/$method_name$',\n");
  printer->Print(
      vars,
      "request,\n"
      "metadata,\n"
      "methodInfo_$method_name$,\n"
      "callback);\n");
  printer->Outdent();
  printer->Outdent();
  printer->Outdent();
  printer->Print("};\n\n\n");
}

void PrintServerStreamingCall(Printer* printer, std::map<string, string> vars) {
  PrintMethodInfo(printer, vars);
  printer->Print(
      vars,
      "/**\n"
      " * @param {!proto.$in$} request The request proto\n"
      " * @param {!Object<string, string>} metadata User defined\n"
      " *     call metadata\n"
      " * @return {!grpc.web.ClientReadableStream<!proto.$out$>}\n"
      " *     The XHR Node Readable Stream\n"
      " */\n"
      "proto.$package_dot$$service_name$Client.prototype.$js_method_name$ =\n");
  printer->Indent();
  printer->Print(
      "  function(request, metadata) {\n"
      "return this.client_.serverStreaming(this.hostname_ +\n");
  printer->Indent();
  printer->Indent();
  printer->Print(vars, "'/$package_dot$$service_name$/$method_name$',\n");
  printer->Print(
      vars,
      "request,\n"
      "metadata,\n"
      "methodInfo_$method_name$);\n");
  printer->Outdent();
  printer->Outdent();
  printer->Outdent();
  printer->Print("};\n\n\n");
}
