/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "doip-server", "index.html", [
    [ "Dependencies", "index.html#autotoc_md41", [
      [ "Getting started", "index.html#autotoc_md42", null ],
      [ "Installation", "index.html#autotoc_md43", null ],
      [ "Installing doctest", "index.html#autotoc_md44", null ]
    ] ],
    [ "Debugging", "index.html#autotoc_md45", [
      [ "Dump UDP", "index.html#autotoc_md46", null ]
    ] ],
    [ "Examples", "index.html#autotoc_md47", null ],
    [ "Acknowledgments", "index.html#autotoc_md48", null ],
    [ "References", "index.html#autotoc_md49", null ],
    [ "Example DoIP Server Tutorial", "md_doc_DoIPServer.html", [
      [ "Overview", "md_doc_DoIPServer.html#autotoc_md11", null ],
      [ "Files of interest", "md_doc_DoIPServer.html#autotoc_md12", [
        [ "ServerModel interface (important callbacks)", "md_doc_DoIPServer.html#autotoc_md13", null ]
      ] ],
      [ "Building the example", "md_doc_DoIPServer.html#autotoc_md14", null ],
      [ "Running the example server", "md_doc_DoIPServer.html#autotoc_md15", null ],
      [ "Customizing UDS behavior", "md_doc_DoIPServer.html#autotoc_md16", null ],
      [ "Integrating a real downstream transport", "md_doc_DoIPServer.html#autotoc_md17", null ],
      [ "Diagram: ServerModel interactions", "md_doc_DoIPServer.html#autotoc_md18", null ],
      [ "Logging and debugging tips", "md_doc_DoIPServer.html#autotoc_md19", null ],
      [ "Troubleshooting common issues", "md_doc_DoIPServer.html#autotoc_md20", null ],
      [ "Documentation / Doxygen", "md_doc_DoIPServer.html#autotoc_md21", null ],
      [ "Message flow and payload types", "md_doc_DoIPServer.html#autotoc_md22", null ],
      [ "Next steps", "md_doc_DoIPServer.html#autotoc_md23", null ]
    ] ],
    [ "ToDo-Transport", "md_doc_internal_ToDo_Transport.html", null ],
    [ "Logging in libdoip", "md_doc_Logging.html", [
      [ "Features", "md_doc_Logging.html#autotoc_md25", null ],
      [ "Usage", "md_doc_Logging.html#autotoc_md26", [
        [ "Code Review - Transport Layer Integration", "md_doc_internal_ToDo_Transport.html#autotoc_md1", [
          [ "✅ <strong>Sehr gut umgesetzt:</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md2", null ],
          [ "⚠️ <strong>Probleme & Verbesserungsvorschläge:</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md3", [
            [ "1. <strong>Duplikation in DoIPConnection</strong> (wie Sie erwähnt haben)", "md_doc_internal_ToDo_Transport.html#autotoc_md4", null ],
            [ "2. <strong>DoIPConnection::receiveMessage() sollte Transport nutzen</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md5", null ],
            [ "3. <strong>DoIPDefaultConnection::closeConnection() sollte Transport schließen</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md6", null ],
            [ "4. <strong>Typ-Aliase fehlen</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md7", null ],
            [ "5. <strong>DoIPConnection.h - Redundante Member</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md8", null ]
          ] ],
          [ "📋 <strong>Empfohlene nächste Schritte:</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md9", null ],
          [ "🎯 <strong>Zusammenfassung:</strong>", "md_doc_internal_ToDo_Transport.html#autotoc_md10", null ]
        ] ],
        [ "Configuration", "md_doc_Logging.html#autotoc_md27", null ],
        [ "Pattern Format", "md_doc_Logging.html#autotoc_md28", null ]
      ] ]
    ] ],
    [ "ToDo-Transport", "md_doc_ToDo_Transport.html", null ],
    [ "Transport Abstraction Layer", "md_doc_Transport.html", [
      [ "Overview", "md_doc_Transport.html#autotoc_md50", null ],
      [ "Architecture", "md_doc_Transport.html#autotoc_md51", [
        [ "Code Review - Transport Layer Integration", "md_doc_ToDo_Transport.html#autotoc_md29", [
          [ "✅ <strong>Sehr gut umgesetzt:</strong>", "md_doc_ToDo_Transport.html#autotoc_md30", null ],
          [ "⚠️ <strong>Probleme & Verbesserungsvorschläge:</strong>", "md_doc_ToDo_Transport.html#autotoc_md31", [
            [ "1. <strong>Duplikation in DoIPConnection</strong> (wie Sie erwähnt haben)", "md_doc_ToDo_Transport.html#autotoc_md32", null ],
            [ "2. <strong>DoIPConnection::receiveMessage() sollte Transport nutzen</strong>", "md_doc_ToDo_Transport.html#autotoc_md33", null ],
            [ "3. <strong>DoIPDefaultConnection::closeConnection() sollte Transport schließen</strong>", "md_doc_ToDo_Transport.html#autotoc_md34", null ],
            [ "4. <strong>Typ-Aliase fehlen</strong>", "md_doc_ToDo_Transport.html#autotoc_md35", null ],
            [ "5. <strong>DoIPConnection.h - Redundante Member</strong>", "md_doc_ToDo_Transport.html#autotoc_md36", null ]
          ] ],
          [ "📋 <strong>Empfohlene nächste Schritte:</strong>", "md_doc_ToDo_Transport.html#autotoc_md37", null ],
          [ "🎯 <strong>Zusammenfassung:</strong>", "md_doc_ToDo_Transport.html#autotoc_md38", null ]
        ] ],
        [ "Class Diagram", "md_doc_Transport.html#autotoc_md52", null ],
        [ "Components", "md_doc_Transport.html#autotoc_md53", [
          [ "<tt>ITransport</tt> Interface", "md_doc_Transport.html#autotoc_md54", null ],
          [ "<tt>TcpTransport</tt> Implementation", "md_doc_Transport.html#autotoc_md55", null ],
          [ "<tt>MockTransport</tt> Implementation", "md_doc_Transport.html#autotoc_md56", null ]
        ] ]
      ] ],
      [ "Integration with DoIPDefaultConnection", "md_doc_Transport.html#autotoc_md57", [
        [ "Current State (Before Refactoring)", "md_doc_Transport.html#autotoc_md58", null ],
        [ "Target State (After Refactoring)", "md_doc_Transport.html#autotoc_md59", null ],
        [ "Migration Steps", "md_doc_Transport.html#autotoc_md60", null ]
      ] ],
      [ "Benefits", "md_doc_Transport.html#autotoc_md61", [
        [ "1. Testability", "md_doc_Transport.html#autotoc_md62", null ],
        [ "2. Extensibility", "md_doc_Transport.html#autotoc_md63", null ],
        [ "3. Separation of Concerns", "md_doc_Transport.html#autotoc_md64", null ]
      ] ],
      [ "Thread Safety", "md_doc_Transport.html#autotoc_md65", null ],
      [ "Performance Considerations", "md_doc_Transport.html#autotoc_md66", null ],
      [ "Error Handling", "md_doc_Transport.html#autotoc_md67", null ],
      [ "Examples", "md_doc_Transport.html#autotoc_md68", [
        [ "Example 1: Testing State Machine Timeout", "md_doc_Transport.html#autotoc_md69", null ],
        [ "Example 2: Testing Invalid Message Handling", "md_doc_Transport.html#autotoc_md70", null ],
        [ "Example 3: Production TCP Usage", "md_doc_Transport.html#autotoc_md71", null ]
      ] ],
      [ "Future Enhancements", "md_doc_Transport.html#autotoc_md72", null ],
      [ "Related Files", "md_doc_Transport.html#autotoc_md73", null ],
      [ "References", "md_doc_Transport.html#autotoc_md74", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"AnsiColors_8h.html",
"DoIPServerModel_8h_source.html",
"classdoip_1_1DoIPClient.html#a469f0d306b3d7ac4520998291e8decfe",
"classdoip_1_1IDownstreamProvider.html",
"md_doc_DoIPServer.html#autotoc_md14",
"namespacedoip_1_1ansi.html#ad89255928e63ea0f7e5a013a769557fc"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';