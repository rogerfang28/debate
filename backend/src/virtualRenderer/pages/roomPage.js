// backend/src/virtualRenderer/pages/roomPage.js
import { PageSchema, ComponentType } from "../../../../protos/wrappers/page-esm-wrapper.mjs";
import { create } from "@bufbuild/protobuf";

export default function roomPage() {
  return create(PageSchema, {
    pageId: "room",
    title: "DebateGraph - Room",
    components: [
      // Navigation Header
      {
        id: "navbar",
        type: ComponentType.CONTAINER,
        style: { 
          customClass: "flex justify-between items-center p-4 bg-gray-900 text-white border-b border-gray-700"
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
                text: "← Home",
                style: { 
                  customClass: "text-blue-400 hover:bg-blue-400/10 px-3 py-2 rounded-lg transition-colors border-none bg-transparent"
                },
                events: { onClick: "goHome" }
              },
              {
                id: "roomTitle",
                type: ComponentType.TEXT,
                text: "🗣️ DebateGraph", // Will be updated with room name
                style: { 
                  customClass: "text-xl font-semibold bg-gradient-to-r from-blue-400 to-purple-400 bg-clip-text text-transparent"
                }
              }
            ]
          },
          {
            id: "navRight",
            type: ComponentType.CONTAINER,
            style: { customClass: "flex items-center gap-2" },
            children: [
              {
                id: "btnToggleMembers",
                type: ComponentType.BUTTON,
                text: "👥 Members",
                style: { 
                  customClass: "px-3 py-2 bg-gray-700 hover:bg-gray-600 rounded-lg transition-colors text-sm"
                },
                events: { onClick: "toggleMembersPanel" }
              },
              {
                id: "btnProfile",
                type: ComponentType.BUTTON,
                text: "👤",
                style: { 
                  customClass: "px-3 py-2 bg-gray-700 hover:bg-gray-600 rounded-lg transition-colors"
                },
                events: { onClick: "goProfile" }
              },
              {
                id: "btnLogout",
                type: ComponentType.BUTTON,
                text: "🚪",
                style: { 
                  customClass: "px-3 py-2 bg-red-600 hover:bg-red-500 rounded-lg transition-colors"
                },
                events: { onClick: "logout" }
              }
            ]
          }
        ]
      },

      // Main Content Container
      {
        id: "mainContent",
        type: ComponentType.CONTAINER,
        style: { 
          customClass: "flex h-screen"
        },
        children: [
          // Graph Canvas Area
          {
            id: "graphContainer",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "flex-1 relative bg-gray-900"
            },
            children: [
              // Graph Canvas (placeholder)
              {
                id: "graphCanvas",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "w-full h-full bg-gradient-to-br from-gray-900 to-gray-800 relative overflow-hidden"
                },
                children: [
                  {
                    id: "graphPlaceholder",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "absolute inset-0 flex items-center justify-center text-gray-400"
                    },
                    children: [
                      {
                        id: "graphMessage",
                        type: ComponentType.TEXT,
                        text: "🎯 Graph visualization will load here",
                        style: { 
                          customClass: "text-lg font-medium"
                        }
                      }
                    ]
                  }
                ]
              },

              // Floating Toolbar
              {
                id: "toolbar",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "absolute top-4 left-4 bg-gray-800/90 backdrop-blur-sm rounded-lg border border-gray-700 p-2 flex gap-2"
                },
                children: [
                  {
                    id: "btnAddNode",
                    type: ComponentType.BUTTON,
                    text: "➕ Node",
                    style: { 
                      customClass: "px-3 py-2 bg-blue-600 hover:bg-blue-500 text-white rounded-md text-sm font-medium transition-colors"
                    },
                    events: { onClick: "addNode" }
                  },
                  {
                    id: "btnAddEdge",
                    type: ComponentType.BUTTON,
                    text: "🔗 Edge",
                    style: { 
                      customClass: "px-3 py-2 bg-green-600 hover:bg-green-500 text-white rounded-md text-sm font-medium transition-colors"
                    },
                    events: { onClick: "addEdge" }
                  },
                  {
                    id: "btnChallenge",
                    type: ComponentType.BUTTON,
                    text: "⚔️ Challenge",
                    style: { 
                      customClass: "px-3 py-2 bg-red-600 hover:bg-red-500 text-white rounded-md text-sm font-medium transition-colors"
                    },
                    events: { onClick: "openChallengeModal" }
                  },
                  {
                    id: "btnZoomIn",
                    type: ComponentType.BUTTON,
                    text: "🔍+",
                    style: { 
                      customClass: "px-3 py-2 bg-gray-600 hover:bg-gray-500 text-white rounded-md text-sm transition-colors"
                    },
                    events: { onClick: "zoomIn" }
                  },
                  {
                    id: "btnZoomOut",
                    type: ComponentType.BUTTON,
                    text: "🔍-",
                    style: { 
                      customClass: "px-3 py-2 bg-gray-600 hover:bg-gray-500 text-white rounded-md text-sm transition-colors"
                    },
                    events: { onClick: "zoomOut" }
                  }
                ]
              },

              // Selection Info Panel (bottom right)
              {
                id: "selectionPanel",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "absolute bottom-4 right-4 bg-gray-800/90 backdrop-blur-sm rounded-lg border border-gray-700 p-4 min-w-64 max-w-sm"
                },
                children: [
                  {
                    id: "selectionTitle",
                    type: ComponentType.TEXT,
                    text: "Selection Info",
                    style: { 
                      customClass: "text-sm font-semibold text-gray-300 mb-2"
                    }
                  },
                  {
                    id: "selectionContent",
                    type: ComponentType.TEXT,
                    text: "Click on a node or edge to see details",
                    style: { 
                      customClass: "text-sm text-gray-400"
                    }
                  }
                ]
              }
            ]
          },

          // Right Sidebar (Members Panel - hidden by default)
          {
            id: "membersSidebar",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "hidden w-80 bg-gray-800 border-l border-gray-700"
            },
            children: [
              {
                id: "membersHeader",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "p-4 border-b border-gray-700 flex justify-between items-center"
                },
                children: [
                  {
                    id: "membersTitle",
                    type: ComponentType.TEXT,
                    text: "👥 Room Members",
                    style: { 
                      customClass: "font-semibold text-white"
                    }
                  },
                  {
                    id: "btnCloseSidebar",
                    type: ComponentType.BUTTON,
                    text: "✕",
                    style: { 
                      customClass: "text-gray-400 hover:text-white p-1 rounded transition-colors border-none bg-transparent"
                    },
                    events: { onClick: "closeMembersPanel" }
                  }
                ]
              },
              {
                id: "membersList",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "p-4"
                },
                children: [
                  {
                    id: "onlineMembers",
                    type: ComponentType.LIST,
                    style: { 
                      customClass: "space-y-2"
                    },
                    items: [
                      { label: "👤 Loading members...", value: "loading" }
                    ]
                  }
                ]
              },
              
              // Room Settings Section
              {
                id: "roomSettings",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "p-4 border-t border-gray-700"
                },
                children: [
                  {
                    id: "settingsTitle",
                    type: ComponentType.TEXT,
                    text: "⚙️ Room Settings",
                    style: { 
                      customClass: "font-semibold text-white mb-3"
                    }
                  },
                  {
                    id: "settingsButtons",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "space-y-2"
                    },
                    children: [
                      {
                        id: "btnInviteCode",
                        type: ComponentType.BUTTON,
                        text: "📋 Copy Invite Code",
                        style: { 
                          customClass: "w-full p-2 bg-blue-600 hover:bg-blue-500 text-white rounded-md text-sm transition-colors"
                        },
                        events: { onClick: "copyInviteCode" }
                      },
                      {
                        id: "btnRoomSettings",
                        type: ComponentType.BUTTON,
                        text: "⚙️ Manage Room",
                        style: { 
                          customClass: "w-full p-2 bg-gray-600 hover:bg-gray-500 text-white rounded-md text-sm transition-colors"
                        },
                        events: { onClick: "openRoomSettings" }
                      }
                    ]
                  }
                ]
              }
            ]
          }
        ]
      },

      // Entity Form Modal (hidden by default)
      {
        id: "entityFormModal",
        type: ComponentType.CONTAINER,
        style: { 
          customClass: "hidden fixed inset-0 bg-black/50 backdrop-blur-sm z-50 flex items-center justify-center"
        },
        children: [
          {
            id: "entityForm",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "bg-gray-800 rounded-lg border border-gray-700 p-6 w-full max-w-md mx-4"
            },
            children: [
              {
                id: "formHeader",
                type: ComponentType.CONTAINER,
                style: { 
                  customClass: "flex justify-between items-center mb-4"
                },
                children: [
                  {
                    id: "formTitle",
                    type: ComponentType.TEXT,
                    text: "Add Node",
                    style: { 
                      customClass: "text-lg font-semibold text-white"
                    }
                  },
                  {
                    id: "btnCloseForm",
                    type: ComponentType.BUTTON,
                    text: "✕",
                    style: { 
                      customClass: "text-gray-400 hover:text-white p-1 rounded transition-colors border-none bg-transparent"
                    },
                    events: { onClick: "closeEntityForm" }
                  }
                ]
              },
              {
                id: "formFields",
                type: ComponentType.FORM,
                name: "entityForm",
                style: { 
                  customClass: "space-y-4"
                },
                children: [
                  {
                    id: "nodeText",
                    type: ComponentType.TEXTAREA,
                    text: "Enter node text...",
                    name: "text",
                    style: { 
                      customClass: "w-full p-3 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent resize-none"
                    },
                    events: { onChange: "updateFormField" }
                  },
                  {
                    id: "nodeType",
                    type: ComponentType.INPUT,
                    text: "Node type (optional)",
                    name: "type",
                    style: { 
                      customClass: "w-full p-3 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    },
                    events: { onChange: "updateFormField" }
                  },
                  {
                    id: "formActions",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "flex gap-3 pt-2"
                    },
                    children: [
                      {
                        id: "btnSubmitForm",
                        type: ComponentType.BUTTON,
                        text: "Create",
                        style: { 
                          customClass: "flex-1 px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white font-medium rounded-lg transition-colors"
                        },
                        events: { onClick: "submitEntityForm" }
                      },
                      {
                        id: "btnCancelForm",
                        type: ComponentType.BUTTON,
                        text: "Cancel",
                        style: { 
                          customClass: "px-4 py-2 bg-gray-600 hover:bg-gray-500 text-white font-medium rounded-lg transition-colors"
                        },
                        events: { onClick: "closeEntityForm" }
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