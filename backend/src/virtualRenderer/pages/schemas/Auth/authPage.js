// // backend/src/virtualRenderer/pages/authPage.js
// import { PageSchema, ComponentType } from "../../../../../../src/gen/page_pb.js";
// import { create } from "@bufbuild/protobuf";

// export default function authPage() {
//   return create(PageSchema, {
//     pageId: "auth",
//     title: "DebateGraph - Sign In",
//     components: [
//       // Page wrapper to add horizontal padding to the whole page
//       {
//         id: "pageWrapper",
//         type: ComponentType.CONTAINER,
//         style: {
//           customClass: "px-4 sm:px-6 lg:px-8"
//         },
//         children: [
//           // Navigation Header
//           {
//             id: "navbar",
//             type: ComponentType.CONTAINER,
//             style: { 
//               customClass: "flex justify-between items-center p-6",
//               bgColor: "bg-gray-900",
//               textColor: "text-white"
//             },
//             children: [
//               {
//                 id: "navLeft",
//                 type: ComponentType.CONTAINER,
//                 style: { customClass: "flex items-center gap-4" },
//                 children: [
//                   {
//                     id: "btnBack",
//                     type: ComponentType.BUTTON,
//                     text: "←",
//                     style: { 
//                       customClass: "text-2xl text-blue-400 hover:bg-blue-400/10 p-2 rounded-lg transition-colors cursor-pointer border-none bg-transparent"
//                     },
//                     events: { onClick: "goHome" }
//                   },
//                   {
//                     id: "pageTitle",
//                     type: ComponentType.TEXT,
//                     text: "🗣️ DebateGraph - Sign In",
//                     style: { 
//                       customClass: "text-2xl font-semibold bg-gradient-to-r from-blue-400 to-purple-400 bg-clip-text text-transparent"
//                     }
//                   }
//                 ]
//               },
//               {
//                 id: "btnGoProfile",
//                 type: ComponentType.BUTTON,
//                 text: "Profile",
//                 style: { 
//                   customClass: "px-4 py-2 bg-gradient-to-r from-blue-600 to-purple-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all"
//                 },
//                 events: { onClick: "goProfile" }
//               }
//             ]
//           },

//           // Main Auth Container
//           {
//             id: "authContainer",
//             type: ComponentType.CONTAINER,
//             style: { 
//               customClass: "max-w-xl mx-auto mt-8"
//             },
//             children: [
//               // Auth Card
//               {
//                 id: "authCard",
//                 type: ComponentType.CARD,
//                 style: { 
//                   customClass: "bg-gradient-to-br from-slate-800 to-slate-700 border border-white/10 shadow-2xl rounded-2xl overflow-hidden"
//                 },
//                 children: [
//                   // Header / Mode Switch
//                   {
//                     id: "authHeader",
//                     type: ComponentType.CONTAINER,
//                     style: { 
//                       customClass: "bg-gradient-to-r from-blue-400 to-purple-400 p-6 text-center text-white"
//                     },
//                     children: [
//                       {
//                         id: "authTitle",
//                         type: ComponentType.TEXT,
//                         text: "Welcome back",
//                         style: { customClass: "text-2xl font-bold mb-1" }
//                       },
//                       {
//                         id: "authSubtitle",
//                         type: ComponentType.TEXT,
//                         text: "Sign in to continue",
//                         style: { customClass: "text-white/90" }
//                       }
//                     ]
//                   },

//                   // Content
//                   {
//                     id: "authContent",
//                     type: ComponentType.CONTAINER,
//                     style: { customClass: "p-6" },
//                     children: [
//                       // Mode Toggle Buttons
//                       {
//                         id: "modeToggle",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "grid grid-cols-2 gap-2 mb-6" },
//                         children: [
//                           {
//                             id: "btnModeSignIn",
//                             type: ComponentType.BUTTON,
//                             text: "Sign In",
//                             style: { 
//                               customClass: "py-2 rounded-lg font-semibold bg-slate-700 hover:bg-slate-600 transition-colors"
//                             },
//                             events: { onClick: "setAuthModeSignIn" } // backend should flip mode & update labels
//                           },
//                           {
//                             id: "btnModeSignUp",
//                             type: ComponentType.BUTTON,
//                             text: "Create Account",
//                             style: { 
//                               customClass: "py-2 rounded-lg font-semibold bg-slate-700 hover:bg-slate-600 transition-colors"
//                             },
//                             events: { onClick: "setAuthModeSignUp" }
//                           }
//                         ]
//                       },

//                       // Error Message (hidden by default)
//                       {
//                         id: "authError",
//                         type: ComponentType.CONTAINER,
//                         style: { 
//                           customClass: "hidden bg-red-500/10 border border-red-500/30 rounded-lg p-3 mb-4 text-red-300 text-sm"
//                         },
//                         children: [
//                           {
//                             id: "authErrorText",
//                             type: ComponentType.TEXT,
//                             text: ""
//                           }
//                         ]
//                       },

//                       // Success Message (hidden by default)
//                       {
//                         id: "authSuccess",
//                         type: ComponentType.CONTAINER,
//                         style: { 
//                           customClass: "hidden bg-green-500/10 border border-green-500/30 rounded-lg p-3 mb-4 text-green-300 text-sm"
//                         },
//                         children: [
//                           {
//                             id: "authSuccessText",
//                             type: ComponentType.TEXT,
//                             text: ""
//                           }
//                         ]
//                       },

//                       // Email
//                       {
//                         id: "emailField",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "mb-4" },
//                         children: [
//                           {
//                             id: "emailLabel",
//                             type: ComponentType.TEXT,
//                             text: "Email",
//                             style: { customClass: "text-slate-200 mb-2 block" }
//                           },
//                           {
//                             id: "emailInput",
//                             type: ComponentType.INPUT,
//                             text: "you@example.com",
//                             style: { 
//                               customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
//                             },
//                             name: "email",
//                             events: { onChange: "setEmail" }
//                           }
//                         ]
//                       },

//                       // Password
//                       {
//                         id: "passwordField",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "mb-4" },
//                         children: [
//                           {
//                             id: "passwordLabel",
//                             type: ComponentType.TEXT,
//                             text: "Password",
//                             style: { customClass: "text-slate-200 mb-2 block" }
//                           },
//                           {
//                             id: "passwordInput",
//                             type: ComponentType.INPUT,
//                             text: "••••••••",
//                             style: { 
//                               customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
//                             },
//                             name: "password",
//                             inputType: "password",
//                             events: { onChange: "setPassword" }
//                           }
//                         ]
//                       },

//                       // Confirm Password (show only in Sign Up mode — backend can toggle visibility via class)
//                       {
//                         id: "confirmPasswordField",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "mb-4 hidden" }, // make visible for sign-up mode
//                         children: [
//                           {
//                             id: "confirmPasswordLabel",
//                             type: ComponentType.TEXT,
//                             text: "Confirm Password",
//                             style: { customClass: "text-slate-200 mb-2 block" }
//                           },
//                           {
//                             id: "confirmPasswordInput",
//                             type: ComponentType.INPUT,
//                             text: "••••••••",
//                             style: { 
//                               customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
//                             },
//                             name: "confirmPassword",
//                             inputType: "password",
//                             events: { onChange: "setConfirmPassword" }
//                           }
//                         ]
//                       },

//                       // Remember / Forgot row
//                       {
//                         id: "rememberForgotRow",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "flex items-center justify-between mb-6 text-sm text-slate-300" },
//                         children: [
//                           {
//                             id: "rememberMe",
//                             type: ComponentType.CONTAINER,
//                             style: { customClass: "flex items-center gap-2" },
//                             children: [
//                               {
//                                 id: "rememberCheckbox",
//                                 type: ComponentType.INPUT,
//                                 inputType: "checkbox",
//                                 name: "rememberMe",
//                                 style: { customClass: "w-4 h-4 bg-slate-700 border border-slate-600 rounded" },
//                                 events: { onChange: "toggleRememberMe" }
//                               },
//                               {
//                                 id: "rememberLabel",
//                                 type: ComponentType.TEXT,
//                                 text: "Remember me"
//                               }
//                             ]
//                           },
//                           {
//                             id: "forgotPassword",
//                             type: ComponentType.BUTTON,
//                             text: "Forgot password?",
//                             style: { customClass: "text-blue-400 hover:underline bg-transparent border-none p-0" },
//                             events: { onClick: "startPasswordReset" }
//                           }
//                         ]
//                       },

//                       // Submit
//                       {
//                         id: "submitRow",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "flex flex-col gap-3" },
//                         children: [
//                           {
//                             id: "btnPrimaryAuth",
//                             type: ComponentType.BUTTON,
//                             text: "Sign In", // backend should switch to "Create Account" in sign-up mode
//                             style: { 
//                               customClass: "w-full py-3 bg-gradient-to-r from-blue-600 to-blue-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all disabled:opacity-50"
//                             },
//                             events: { onClick: "submitAuth" } // backend should read mode + fields and act
//                           },
//                           {
//                             id: "orDivider",
//                             type: ComponentType.CONTAINER,
//                             style: { customClass: "flex items-center gap-3 my-2" },
//                             children: [
//                               { id: "lineL", type: ComponentType.CONTAINER, style: { customClass: "h-px bg-slate-600 flex-1" } },
//                               { id: "orText", type: ComponentType.TEXT, text: "or", style: { customClass: "text-slate-400 text-sm" } },
//                               { id: "lineR", type: ComponentType.CONTAINER, style: { customClass: "h-px bg-slate-600 flex-1" } }
//                             ]
//                           },
//                           {
//                             id: "btnGoogle",
//                             type: ComponentType.BUTTON,
//                             text: "Continue with Google",
//                             style: { 
//                               customClass: "w-full py-3 bg-slate-700 hover:bg-slate-600 text-white font-semibold rounded-lg transition-colors"
//                             },
//                             events: { onClick: "continueWithGoogle" }
//                           }
//                         ]
//                       },

//                       // Footer: link to switch modes
//                       {
//                         id: "switchModeRow",
//                         type: ComponentType.CONTAINER,
//                         style: { customClass: "mt-6 text-center text-sm text-slate-300" },
//                         children: [
//                           {
//                             id: "switchModeText",
//                             type: ComponentType.TEXT,
//                             text: "Don't have an account?"
//                           },
//                           {
//                             id: "switchModeBtn",
//                             type: ComponentType.BUTTON,
//                             text: "Create one",
//                             style: { customClass: "ml-2 text-blue-400 hover:underline bg-transparent border-none p-0" },
//                             events: { onClick: "setAuthModeSignUp" }
//                           }
//                         ]
//                       }
//                     ]
//                   }
//                 ]
//               }
//             ]
//           }
//         ]
//       }
//     ]
//   });
// }
