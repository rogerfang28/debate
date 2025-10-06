// backend/src/virtualRenderer/pages/loginPage.js
import { PageSchema, ComponentType } from "../../../../../../src/gen/js/page_pb.js";
import { create } from "@bufbuild/protobuf";

export default function loginPage() {
  return create(PageSchema, {
    pageId: "login",
    title: "DebateGraph - Sign In",
    components: [
      // Page wrapper to add horizontal padding to the whole page
      {
        id: "pageWrapper",
        type: ComponentType.CONTAINER,
        style: {
          customClass: "px-4 sm:px-6 lg:px-8"
        },
        children: [
          // Navigation Header
          {
            id: "navbar",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "flex justify-between items-center p-6",
              bgColor: "bg-gray-900",
              textColor: "text-white"
            },
            children: [
              {
                id: "navLeft",
                type: ComponentType.CONTAINER,
                style: { customClass: "flex items-center gap-4" },
                children: [
                  {
                    id: "btnBack",
                    type: ComponentType.BUTTON,
                    text: "←",
                    style: { 
                      customClass: "text-2xl text-blue-400 hover:bg-blue-400/10 p-2 rounded-lg transition-colors cursor-pointer border-none bg-transparent"
                    },
                    events: { onClick: "goHome" }
                  },
                  {
                    id: "pageTitle",
                    type: ComponentType.TEXT,
                    text: "🗣️ DebateGraph - Sign In",
                    style: { 
                      customClass: "text-2xl font-semibold bg-gradient-to-r from-blue-400 to-purple-400 bg-clip-text text-transparent"
                    }
                  }
                ]
              },
            //   {
            //     id: "btnGoProfile",
            //     type: ComponentType.BUTTON,
            //     text: "Profile",
            //     style: { 
            //       customClass: "px-4 py-2 bg-gradient-to-r from-blue-600 to-purple-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all"
            //     },
            //     events: { onClick: "goProfile" }
            //   }
            ]
          },

          // Main Login Container
          {
            id: "loginContainer",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "max-w-xl mx-auto mt-8"
            },
            children: [
              // Login Card
              {
                id: "loginCard",
                type: ComponentType.CARD,
                style: { 
                  customClass: "bg-gradient-to-br from-slate-800 to-slate-700 border border-white/10 shadow-2xl rounded-2xl overflow-hidden"
                },
                children: [
                  // Header
                  {
                    id: "loginHeader",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "bg-gradient-to-r from-blue-400 to-purple-400 p-6 text-center text-white"
                    },
                    children: [
                      {
                        id: "loginTitle",
                        type: ComponentType.TEXT,
                        text: "Welcome back",
                        style: { customClass: "text-2xl font-bold mb-1" }
                      },
                      {
                        id: "loginSubtitle",
                        type: ComponentType.TEXT,
                        text: "Sign in to continue",
                        style: { customClass: "text-white/90" }
                      }
                    ]
                  },

                  // Content
                  {
                    id: "loginContent",
                    type: ComponentType.CONTAINER,
                    style: { customClass: "p-6" },
                    children: [
                      // Error Message (hidden by default)
                      {
                        id: "loginError",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "hidden bg-red-500/10 border border-red-500/30 rounded-lg p-3 mb-4 text-red-300 text-sm"
                        },
                        children: [
                          {
                            id: "loginErrorText",
                            type: ComponentType.TEXT,
                            text: ""
                          }
                        ]
                      },

                      // Success Message (hidden by default)
                      {
                        id: "loginSuccess",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "hidden bg-green-500/10 border border-green-500/30 rounded-lg p-3 mb-4 text-green-300 text-sm"
                        },
                        children: [
                          {
                            id: "loginSuccessText",
                            type: ComponentType.TEXT,
                            text: ""
                          }
                        ]
                      },

                      // Email
                      {
                        id: "emailField",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mb-4" },
                        children: [
                          {
                            id: "emailLabel",
                            type: ComponentType.TEXT,
                            text: "Email",
                            style: { customClass: "text-slate-200 mb-2 block" }
                          },
                          {
                            id: "emailInput",
                            type: ComponentType.INPUT,
                            text: "you@example.com",
                            style: { 
                              customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                            },
                            name: "email",
                            events: { onChange: "setEmail" }
                          }
                        ]
                      },

                      // Password
                      {
                        id: "passwordField",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mb-4" },
                        children: [
                          {
                            id: "passwordLabel",
                            type: ComponentType.TEXT,
                            text: "Password",
                            style: { customClass: "text-slate-200 mb-2 block" }
                          },
                          {
                            id: "passwordInput",
                            type: ComponentType.INPUT,
                            text: "••••••••",
                            style: { 
                              customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                            },
                            name: "password",
                            inputType: "password",
                            events: { onChange: "setPassword" }
                          }
                        ]
                      },

                      // Remember / Forgot row
                      {
                        id: "rememberForgotRow",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "flex items-center justify-between mb-6 text-sm text-slate-300" },
                        children: [
                          {
                            id: "rememberMe",
                            type: ComponentType.CONTAINER,
                            style: { customClass: "flex items-center gap-2" },
                            children: [
                              {
                                id: "rememberCheckbox",
                                type: ComponentType.INPUT,
                                inputType: "checkbox",
                                name: "rememberMe",
                                style: { customClass: "w-4 h-4 bg-slate-700 border border-slate-600 rounded" },
                                events: { onChange: "toggleRememberMe" }
                              },
                              {
                                id: "rememberLabel",
                                type: ComponentType.TEXT,
                                text: "Remember me"
                              }
                            ]
                          },
                          {
                            id: "forgotPassword",
                            type: ComponentType.BUTTON,
                            text: "Forgot password?",
                            style: { customClass: "text-blue-400 hover:underline bg-transparent border-none p-0" },
                            events: { onClick: "startPasswordReset" }
                          }
                        ]
                      },

                      // Submit
                      {
                        id: "submitRow",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "flex flex-col gap-3" },
                        children: [
                          {
                            id: "btnSignIn",
                            type: ComponentType.BUTTON,
                            text: "Sign In",
                            style: { 
                              customClass: "w-full py-3 bg-gradient-to-r from-blue-600 to-blue-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all disabled:opacity-50"
                            },
                            events: {
                                onClick: JSON.stringify({
                                actionId: "submitLogIn",
                                collectFrom: ["emailInput","passwordInput"]
                                })
                            }
                          },
                          {
                            id: "orDivider",
                            type: ComponentType.CONTAINER,
                            style: { customClass: "flex items-center gap-3 my-2" },
                            children: [
                              { id: "lineL", type: ComponentType.CONTAINER, style: { customClass: "h-px bg-slate-600 flex-1" } },
                              { id: "orText", type: ComponentType.TEXT, text: "or", style: { customClass: "text-slate-400 text-sm" } },
                              { id: "lineR", type: ComponentType.CONTAINER, style: { customClass: "h-px bg-slate-600 flex-1" } }
                            ]
                          },
                          {
                            id: "btnGoogle",
                            type: ComponentType.BUTTON,
                            text: "Continue with Google",
                            style: { 
                              customClass: "w-full py-3 bg-slate-700 hover:bg-slate-600 text-white font-semibold rounded-lg transition-colors"
                            },
                            events: { onClick: "continueWithGoogle" }
                          }
                        ]
                      },

                      // Footer: link to signup
                      {
                        id: "switchModeRow",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mt-6 text-center text-sm text-slate-300" },
                        children: [
                          {
                            id: "switchModeText",
                            type: ComponentType.TEXT,
                            text: "Don't have an account?"
                          },
                          {
                            id: "switchModeBtn",
                            type: ComponentType.BUTTON,
                            text: "Create one",
                            style: { customClass: "ml-2 text-blue-400 hover:underline bg-transparent border-none p-0" },
                            events: { onClick: "goSignUp" }
                          }
                        ]
                      }
                    ]
                  }
                ]
              }
            ]
          }
        ]
      }
    ]
  });
}
