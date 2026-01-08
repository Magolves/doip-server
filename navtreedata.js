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
    [ "Features", "index.html#autotoc_md53", null ],
    [ "Dependencies", "index.html#autotoc_md85", [
      [ "Getting started", "index.html#autotoc_md86", [
        [ "Examples", "index.html#autotoc_md89", null ]
      ] ],
      [ "Installation", "index.html#autotoc_md90", null ],
      [ "Installing doctest", "index.html#autotoc_md91", null ]
    ] ],
    [ "Debugging", "index.html#autotoc_md92", [
      [ "Dump UDP", "index.html#autotoc_md93", null ]
    ] ],
    [ "Examples", "index.html#autotoc_md94", null ],
    [ "Acknowledgments", "index.html#autotoc_md95", null ],
    [ "References", "index.html#autotoc_md96", null ],
    [ "Example DoIP Server Tutorial", "md_doc_DoIPServer.html", [
      [ "Overview", "md_doc_DoIPServer.html#autotoc_md6", null ],
      [ "Files of interest", "md_doc_DoIPServer.html#autotoc_md7", [
        [ "ServerModel interface (important callbacks)", "md_doc_DoIPServer.html#autotoc_md8", null ]
      ] ],
      [ "Building the example", "md_doc_DoIPServer.html#autotoc_md9", null ],
      [ "Running the example server", "md_doc_DoIPServer.html#autotoc_md10", null ],
      [ "Customizing UDS behavior", "md_doc_DoIPServer.html#autotoc_md11", null ],
      [ "Integrating a real downstream transport", "md_doc_DoIPServer.html#autotoc_md12", null ],
      [ "Diagram: ServerModel interactions", "md_doc_DoIPServer.html#autotoc_md13", null ],
      [ "Logging and debugging tips", "md_doc_DoIPServer.html#autotoc_md14", null ],
      [ "Troubleshooting common issues", "md_doc_DoIPServer.html#autotoc_md15", null ],
      [ "Documentation / Doxygen", "md_doc_DoIPServer.html#autotoc_md16", null ],
      [ "Message flow and payload types", "md_doc_DoIPServer.html#autotoc_md17", null ],
      [ "Next steps", "md_doc_DoIPServer.html#autotoc_md18", null ]
    ] ],
    [ "Logging in libdoip", "md_doc_Logging.html", [
      [ "Features", "md_doc_Logging.html#autotoc_md1", null ],
      [ "Usage", "md_doc_Logging.html#autotoc_md2", [
        [ "Configuration", "md_doc_Logging.html#autotoc_md3", null ],
        [ "Pattern Format", "md_doc_Logging.html#autotoc_md4", null ]
      ] ]
    ] ],
    [ "UDS Security Access Implementation Guide", "md_doc_SecurityAccess.html", [
      [ "Overview", "md_doc_SecurityAccess.html#autotoc_md47", null ],
      [ "Architecture", "md_doc_SecurityAccess.html#autotoc_md48", [
        [ "Components", "md_doc_SecurityAccess.html#autotoc_md49", null ]
      ] ],
      [ "Security Access Flow", "md_doc_SecurityAccess.html#autotoc_md50", [
        [ "Standard Workflow", "md_doc_SecurityAccess.html#autotoc_md51", null ],
        [ "Security Levels", "md_doc_SecurityAccess.html#autotoc_md52", null ]
      ] ],
      [ "Implementation Guide", "md_doc_SecurityAccess.html#autotoc_md54", [
        [ "Step 1: Implement Your UDS Model", "md_doc_SecurityAccess.html#autotoc_md55", null ],
        [ "Step 2: Common Seed-Key Algorithms", "md_doc_SecurityAccess.html#autotoc_md56", [
          [ "Simple XOR + Addition (Educational)", "md_doc_SecurityAccess.html#autotoc_md57", null ],
          [ "Bit Rotation + XOR (Moderate)", "md_doc_SecurityAccess.html#autotoc_md58", null ],
          [ "Multi-stage Transformation (Advanced)", "md_doc_SecurityAccess.html#autotoc_md59", null ]
        ] ],
        [ "Step 3: Configure Security Parameters", "md_doc_SecurityAccess.html#autotoc_md60", null ]
      ] ],
      [ "ISO 14229 Compliance", "md_doc_SecurityAccess.html#autotoc_md61", [
        [ "Required Behaviors", "md_doc_SecurityAccess.html#autotoc_md62", null ],
        [ "Negative Response Codes", "md_doc_SecurityAccess.html#autotoc_md63", null ]
      ] ],
      [ "Testing Examples", "md_doc_SecurityAccess.html#autotoc_md64", [
        [ "Test Case 1: Successful Unlock", "md_doc_SecurityAccess.html#autotoc_md65", null ],
        [ "Test Case 2: Failed Attempts", "md_doc_SecurityAccess.html#autotoc_md66", null ]
      ] ],
      [ "Integration with DoIP Server", "md_doc_SecurityAccess.html#autotoc_md67", null ],
      [ "Security Best Practices", "md_doc_SecurityAccess.html#autotoc_md68", [
        [ "DO ✅", "md_doc_SecurityAccess.html#autotoc_md69", null ],
        [ "DON'T ❌", "md_doc_SecurityAccess.html#autotoc_md70", null ]
      ] ],
      [ "Troubleshooting", "md_doc_SecurityAccess.html#autotoc_md71", [
        [ "\"RequestSequenceError\" on sendKey", "md_doc_SecurityAccess.html#autotoc_md72", null ],
        [ "\"ExceededNumberOfAttempts\" on requestSeed", "md_doc_SecurityAccess.html#autotoc_md73", null ],
        [ "Key calculation returns wrong value", "md_doc_SecurityAccess.html#autotoc_md74", null ],
        [ "Security unlocked but still getting \"SecurityAccessDenied\"", "md_doc_SecurityAccess.html#autotoc_md75", null ]
      ] ],
      [ "Advanced Topics", "md_doc_SecurityAccess.html#autotoc_md76", [
        [ "Multiple Security Levels", "md_doc_SecurityAccess.html#autotoc_md77", null ],
        [ "Event Monitoring", "md_doc_SecurityAccess.html#autotoc_md78", null ],
        [ "Session-Specific Security", "md_doc_SecurityAccess.html#autotoc_md79", null ]
      ] ],
      [ "Difference between SID 0x27 and 0x84", "md_doc_SecurityAccess.html#autotoc_md80", [
        [ "<strong>1. SID 0x27: Security Access</strong>", "md_doc_SecurityAccess.html#autotoc_md82", null ],
        [ "<strong>2. SID 0x84: Secured Access</strong>", "md_doc_SecurityAccess.html#autotoc_md84", null ],
        [ "<strong>Key Differences</strong>", "md_doc_SecurityAccess.html#autotoc_md88", null ],
        [ "<strong>When to Use Which?</strong>", "md_doc_SecurityAccess.html#autotoc_md98", null ]
      ] ],
      [ "Security Levels and Servcies", "md_doc_SecurityAccess.html#autotoc_md99", [
        [ "<strong>1. No Standardized Security Levels in ISO 14229-1</strong>", "md_doc_SecurityAccess.html#autotoc_md101", null ],
        [ "<strong>2. Manufacturer-Specific Rules</strong>", "md_doc_SecurityAccess.html#autotoc_md103", null ],
        [ "<strong>3. Common Industry Practices (Examples)</strong>", "md_doc_SecurityAccess.html#autotoc_md105", null ],
        [ "<strong>4. How to Determine Requirements</strong>", "md_doc_SecurityAccess.html#autotoc_md107", null ],
        [ "<strong>5. Example: Volkswagen (ODIS)</strong>", "md_doc_SecurityAccess.html#autotoc_md109", null ],
        [ "<strong>Key Takeaway</strong>", "md_doc_SecurityAccess.html#autotoc_md111", null ]
      ] ],
      [ "References", "md_doc_SecurityAccess.html#autotoc_md112", null ]
    ] ],
    [ "Transport Abstraction Layer", "md_doc_Transport.html", [
      [ "Overview", "md_doc_Transport.html#autotoc_md21", null ],
      [ "Architecture", "md_doc_Transport.html#autotoc_md22", [
        [ "Class Diagram", "md_doc_Transport.html#autotoc_md23", null ],
        [ "Components", "md_doc_Transport.html#autotoc_md24", [
          [ "<tt>ITransport</tt> Interface", "md_doc_Transport.html#autotoc_md25", null ],
          [ "<tt>TcpTransport</tt> Implementation", "md_doc_Transport.html#autotoc_md26", null ],
          [ "<tt>MockTransport</tt> Implementation", "md_doc_Transport.html#autotoc_md27", null ]
        ] ]
      ] ],
      [ "Integration with DoIPDefaultConnection", "md_doc_Transport.html#autotoc_md28", [
        [ "Current State (Before Refactoring)", "md_doc_Transport.html#autotoc_md29", null ],
        [ "Target State (After Refactoring)", "md_doc_Transport.html#autotoc_md30", null ],
        [ "Migration Steps", "md_doc_Transport.html#autotoc_md31", null ]
      ] ],
      [ "Benefits", "md_doc_Transport.html#autotoc_md32", [
        [ "1. Testability", "md_doc_Transport.html#autotoc_md33", null ],
        [ "2. Extensibility", "md_doc_Transport.html#autotoc_md34", null ],
        [ "3. Separation of Concerns", "md_doc_Transport.html#autotoc_md35", null ]
      ] ],
      [ "Thread Safety", "md_doc_Transport.html#autotoc_md37", null ],
      [ "Performance Considerations", "md_doc_Transport.html#autotoc_md38", null ],
      [ "Error Handling", "md_doc_Transport.html#autotoc_md39", null ],
      [ "Examples", "md_doc_Transport.html#autotoc_md40", [
        [ "Example 1: Testing State Machine Timeout", "md_doc_Transport.html#autotoc_md41", null ],
        [ "Example 2: Testing Invalid Message Handling", "md_doc_Transport.html#autotoc_md42", null ],
        [ "Example 3: Production TCP Usage", "md_doc_Transport.html#autotoc_md43", null ]
      ] ],
      [ "Future Enhancements", "md_doc_Transport.html#autotoc_md44", null ],
      [ "Related Files", "md_doc_Transport.html#autotoc_md45", null ],
      [ "References", "md_doc_Transport.html#autotoc_md46", null ]
    ] ],
    [ "Todo List", "todo.html", null ],
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
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Enumerations", "globals_enum.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"CanIsoTpProvider_8h.html",
"MockTransport_8cpp.html",
"classdoip_1_1MockConnectionTransport.html#ac6e30cca7156a56bc89d06485a31ddf7",
"namespacedoip.html#a31743d19910bf28ebad1ac0c6c6e4df9a3237ae26902ad4d09416d7c8e2f64159"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';