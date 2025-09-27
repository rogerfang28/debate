// backend/src/virtualRenderer/pages/signupPage.js
import { PageSchema, ComponentType } from "../../../../../../src/gen/js/page_pb.js";
import { create } from "@bufbuild/protobuf";

export default function signupPage() {
  return create(PageSchema, {
    pageId: "signup",
    title: "DebateGraph - Create Account",
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
                    text: "🗣️ DebateGraph - Create Account",
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

          // Main Signup Container
          {
            id: "signupContainer",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "max-w-xl mx-auto mt-8"
            },
            children: [
              // Signup Card
              {
                id: "signupCard",
                type: ComponentType.CARD,
                style: { 
                  customClass: "bg-gradient-to-br from-slate-800 to-slate-700 border border-white/10 shadow-2xl rounded-2xl overflow-hidden"
                },
                children: [
                  // Header
                  {
                    id: "signupHeader",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "bg-gradient-to-r from-blue-400 to-purple-400 p-6 text-center text-white"
                    },
                    children: [
                      {
                        id: "signupTitle",
                        type: ComponentType.TEXT,
                        text: "Join DebateGraph",
                        style: { customClass: "text-2xl font-bold mb-1" }
                      },
                      {
                        id: "signupSubtitle",
                        type: ComponentType.TEXT,
                        text: "Create your account to get started",
                        style: { customClass: "text-white/90" }
                      }
                    ]
                  },

                  // Content
                  {
                    id: "signupContent",
                    type: ComponentType.CONTAINER,
                    style: { customClass: "p-6" },
                    children: [
                      // Error Message (hidden by default)
                      {
                        id: "signupError",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "hidden bg-red-500/10 border border-red-500/30 rounded-lg p-3 mb-4 text-red-300 text-sm"
                        },
                        children: [
                          {
                            id: "signupErrorText",
                            type: ComponentType.TEXT,
                            text: ""
                          }
                        ]
                      },

                      // Success Message (hidden by default)
                      {
                        id: "signupSuccess",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "hidden bg-green-500/10 border border-green-500/30 rounded-lg p-3 mb-4 text-green-300 text-sm"
                        },
                        children: [
                          {
                            id: "signupSuccessText",
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

                      // Username
                      {
                        id: "usernameField",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mb-4" },
                        children: [
                          {
                            id: "usernameLabel",
                            type: ComponentType.TEXT,
                            text: "Username",
                            style: { customClass: "text-slate-200 mb-2 block" }
                          },
                          {
                            id: "usernameInput",
                            type: ComponentType.INPUT,
                            text: "your_username",
                            style: { 
                              customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                            },
                            name: "username",
                            events: { onChange: "setUsername" }
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

                      // Confirm Password
                      {
                        id: "confirmPasswordField",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mb-4" },
                        children: [
                          {
                            id: "confirmPasswordLabel",
                            type: ComponentType.TEXT,
                            text: "Confirm Password",
                            style: { customClass: "text-slate-200 mb-2 block" }
                          },
                          {
                            id: "confirmPasswordInput",
                            type: ComponentType.INPUT,
                            text: "••••••••",
                            style: { 
                              customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                            },
                            name: "confirmPassword",
                            inputType: "password",
                            events: { onChange: "setConfirmPassword" }
                          }
                        ]
                      },

                      // Terms and Conditions
                      {
                        id: "termsRow",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mb-6" },
                        children: [
                          {
                            id: "termsCheckbox",
                            type: ComponentType.CONTAINER,
                            style: { customClass: "flex items-start gap-3 text-sm text-slate-300" },
                            children: [
                              {
                                id: "termsCheckboxInput",
                                type: ComponentType.INPUT,
                                inputType: "checkbox",
                                name: "acceptTerms",
                                style: { customClass: "w-4 h-4 bg-slate-700 border border-slate-600 rounded mt-0.5" },
                                events: { onChange: "toggleAcceptTerms" }
                              },
                              {
                                id: "termsText",
                                type: ComponentType.CONTAINER,
                                style: { customClass: "flex-1" },
                                children: [
                                  {
                                    id: "termsLabel",
                                    type: ComponentType.TEXT,
                                    text: "I agree to the "
                                  },
                                  {
                                    id: "termsLink",
                                    type: ComponentType.BUTTON,
                                    text: "Terms of Service",
                                    style: { customClass: "text-blue-400 hover:underline bg-transparent border-none p-0" },
                                    events: { onClick: "showTerms" }
                                  },
                                  {
                                    id: "termsAnd",
                                    type: ComponentType.TEXT,
                                    text: " and "
                                  },
                                  {
                                    id: "privacyLink",
                                    type: ComponentType.BUTTON,
                                    text: "Privacy Policy",
                                    style: { customClass: "text-blue-400 hover:underline bg-transparent border-none p-0" },
                                    events: { onClick: "showPrivacy" }
                                  }
                                ]
                              }
                            ]
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
                            id: "btnCreateAccount",
                            type: ComponentType.BUTTON,
                            text: "Create Account",
                            style: { 
                              customClass: "w-full py-3 bg-gradient-to-r from-blue-600 to-blue-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all disabled:opacity-50"
                            },
                            
                            events: {
                                onClick: JSON.stringify({
                                actionId: "submitSignUp",
                                collectFrom: ["emailInput","passwordInput","confirmPasswordInput","usernameInput"]
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

                      // Footer: link to login
                      {
                        id: "switchModeRow",
                        type: ComponentType.CONTAINER,
                        style: { customClass: "mt-6 text-center text-sm text-slate-300" },
                        children: [
                          {
                            id: "switchModeText",
                            type: ComponentType.TEXT,
                            text: "Already have an account?"
                          },
                          {
                            id: "switchModeBtn",
                            type: ComponentType.BUTTON,
                            text: "Sign in",
                            style: { customClass: "ml-2 text-blue-400 hover:underline bg-transparent border-none p-0" },
                            events: { onClick: "goLogin" }
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
