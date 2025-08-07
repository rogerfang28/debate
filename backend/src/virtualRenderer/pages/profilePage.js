// backend/src/virtualRenderer/pages/profilePage.js
import { PageSchema, ComponentType } from "../../../../src/gen/page_pb.js";
import { create } from "@bufbuild/protobuf";

export default function profilePage() {
  return create(PageSchema, {
    pageId: "profile",
    title: "DebateGraph - Profile",
    components: [
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
                text: "🗣️ DebateGraph - Profile",
                style: { 
                  customClass: "text-2xl font-semibold bg-gradient-to-r from-blue-400 to-purple-400 bg-clip-text text-transparent"
                }
              }
            ]
          },
          {
            id: "btnLogout",
            type: ComponentType.BUTTON,
            text: "Logout",
            style: { 
              customClass: "px-4 py-2 bg-gradient-to-r from-red-600 to-red-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all"
            },
            events: { onClick: "logout" }
          }
        ]
      },

      // Main Profile Container
      {
        id: "profileContainer",
        type: ComponentType.CONTAINER,
        style: { 
          customClass: "max-w-4xl mx-auto p-12"
        },
        children: [
          // Profile Card
          {
            id: "profileCard",
            type: ComponentType.CARD,
            style: { 
              customClass: "bg-gradient-to-br from-slate-800 to-slate-700 border border-white/10 shadow-2xl rounded-2xl overflow-hidden"
            },
            children: [
              // Profile Header Section
              {
                id: "profileHeader",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "bg-gradient-to-r from-blue-400 to-purple-400 p-8 text-center text-white"
                },
                children: [
                  {
                    id: "avatarIcon",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "w-20 h-20 bg-white/20 rounded-full flex items-center justify-center mx-auto mb-4 text-4xl"
                    },
                    children: [
                      {
                        id: "avatarEmoji",
                        type: ComponentType.TEXT,
                        text: "👤"
                      }
                    ]
                  },
                  {
                    id: "userName",
                    type: ComponentType.TEXT,
                    text: "Loading...", // Will be populated dynamically
                    style: { 
                      customClass: "text-3xl font-bold mb-2"
                    }
                  },
                  {
                    id: "userRole",
                    type: ComponentType.TEXT,
                    text: "Debate Participant",
                    style: { 
                      customClass: "text-lg text-white/80"
                    }
                  }
                ]
              },

              // Profile Content Section
              {
                id: "profileContent",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "p-8"
                },
                children: [
                  // Error Message (hidden by default)
                  {
                    id: "errorMessage",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "hidden bg-red-500/10 border border-red-500/30 rounded-lg p-3 mb-6 text-red-300 text-sm"
                    },
                    children: [
                      {
                        id: "errorText",
                        type: ComponentType.TEXT,
                        text: ""
                      }
                    ]
                  },

                  // Username Edit Section
                  {
                    id: "usernameSection",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "mb-8"
                    },
                    children: [
                      {
                        id: "usernameHeader",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "flex justify-between items-center mb-4"
                        },
                        children: [
                          {
                            id: "usernameLabel",
                            type: ComponentType.TEXT,
                            text: "Username",
                            style: { 
                              customClass: "text-lg font-semibold text-slate-200"
                            }
                          },
                          {
                            id: "btnEdit",
                            type: ComponentType.BUTTON,
                            text: "✏️ Edit",
                            style: { 
                              customClass: "px-3 py-1.5 bg-gradient-to-r from-green-600 to-green-500 text-white text-sm font-semibold rounded-md hover:transform hover:-translate-y-0.5 transition-all"
                            },
                            events: { onClick: "toggleEditMode" }
                          }
                        ]
                      },
                      
                      // Username Display/Edit Form
                      {
                        id: "usernameForm",
                        type: ComponentType.CONTAINER,
                        children: [
                          {
                            id: "usernameInput",
                            type: ComponentType.INPUT,
                            text: "Enter username",
                            style: { 
                              customClass: "w-full p-3 bg-slate-700 border border-slate-600 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                            },
                            name: "username",
                            events: { onChange: "updateUsername" }
                          },
                          {
                            id: "saveButtonContainer",
                            type: ComponentType.CONTAINER,
                            style: { 
                              customClass: "flex gap-3 mt-4"
                            },
                            children: [
                              {
                                id: "btnSaveProfile",
                                type: ComponentType.BUTTON,
                                text: "💾 Save Changes",
                                style: { 
                                  customClass: "px-4 py-2 bg-gradient-to-r from-blue-600 to-blue-500 text-white font-semibold rounded-lg hover:transform hover:-translate-y-0.5 transition-all disabled:opacity-50"
                                },
                                events: { onClick: "saveProfile" }
                              },
                              {
                                id: "btnCancel",
                                type: ComponentType.BUTTON,
                                text: "Cancel",
                                style: { 
                                  customClass: "px-4 py-2 bg-slate-600 text-white font-semibold rounded-lg hover:bg-slate-500 transition-colors"
                                },
                                events: { onClick: "cancelEdit" }
                              }
                            ]
                          }
                        ]
                      }
                    ]
                  },

                  // Account Information Section
                  {
                    id: "accountInfo",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "mb-8"
                    },
                    children: [
                      {
                        id: "accountInfoTitle",
                        type: ComponentType.TEXT,
                        text: "Account Information",
                        style: { 
                          customClass: "text-lg font-semibold text-slate-200 mb-4"
                        }
                      },
                      {
                        id: "emailInfo",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "flex justify-between items-center py-3 border-b border-slate-600"
                        },
                        children: [
                          {
                            id: "emailLabel",
                            type: ComponentType.TEXT,
                            text: "Email",
                            style: { 
                              customClass: "text-slate-300 font-medium"
                            }
                          },
                          {
                            id: "emailValue",
                            type: ComponentType.TEXT,
                            text: "Loading...", // Will be populated dynamically
                            style: { 
                              customClass: "text-slate-200"
                            }
                          }
                        ]
                      },
                      {
                        id: "joinDateInfo",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "flex justify-between items-center py-3"
                        },
                        children: [
                          {
                            id: "joinDateLabel",
                            type: ComponentType.TEXT,
                            text: "Member Since",
                            style: { 
                              customClass: "text-slate-300 font-medium"
                            }
                          },
                          {
                            id: "joinDateValue",
                            type: ComponentType.TEXT,
                            text: "Loading...", // Will be populated dynamically
                            style: { 
                              customClass: "text-slate-200"
                            }
                          }
                        ]
                      }
                    ]
                  },

                  // Quick Actions Section
                  {
                    id: "quickActions",
                    type: ComponentType.CONTAINER,
                    children: [
                      {
                        id: "quickActionsTitle",
                        type: ComponentType.TEXT,
                        text: "Quick Actions",
                        style: { 
                          customClass: "text-lg font-semibold text-slate-200 mb-4"
                        }
                      },
                      {
                        id: "actionsGrid",
                        type: ComponentType.CONTAINER,
                        style: { 
                          customClass: "grid grid-cols-2 gap-4"
                        },
                        children: [
                          {
                            id: "btnGoHome",
                            type: ComponentType.BUTTON,
                            text: "🏠 Home",
                            style: { 
                              customClass: "p-4 bg-slate-700 hover:bg-slate-600 rounded-lg transition-colors text-center font-medium"
                            },
                            events: { onClick: "goHome" }
                          },
                          {
                            id: "btnViewRooms",
                            type: ComponentType.BUTTON,
                            text: "🏠 My Rooms",
                            style: { 
                              customClass: "p-4 bg-slate-700 hover:bg-slate-600 rounded-lg transition-colors text-center font-medium"
                            },
                            events: { onClick: "goHome" }
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