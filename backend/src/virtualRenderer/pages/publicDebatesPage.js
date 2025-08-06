// backend/src/virtualRenderer/pages/publicDebatesPage.js
import { PageSchema, ComponentType } from "../../../../protos/wrappers/page-esm-wrapper.mjs";
import { create } from "@bufbuild/protobuf";

export default function publicDebatesPage() {
  return create(PageSchema, {
    pageId: "publicDebates",
    title: "DebateGraph - Public Debates",
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
                id: "pageTitle",
                type: ComponentType.TEXT,
                text: "🌍 Public Debates",
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
                id: "btnProfile",
                type: ComponentType.BUTTON,
                text: "👤 Profile",
                style: { 
                  customClass: "px-3 py-2 bg-gray-700 hover:bg-gray-600 rounded-lg transition-colors text-sm"
                },
                events: { onClick: "goProfile" }
              },
              {
                id: "btnLogout",
                type: ComponentType.BUTTON,
                text: "🚪 Logout",
                style: { 
                  customClass: "px-3 py-2 bg-red-600 hover:bg-red-500 rounded-lg transition-colors text-sm"
                },
                events: { onClick: "logout" }
              }
            ]
          }
        ]
      },

      // Main Content
      {
        id: "mainContent",
        type: ComponentType.CONTAINER,
        style: { 
          customClass: "max-w-7xl mx-auto p-6"
        },
        children: [
          // Header Section
          {
            id: "headerSection",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "text-center mb-8"
            },
            children: [
              {
                id: "pageHeading",
                type: ComponentType.TEXT,
                text: "🌍 Explore Public Debates",
                style: { 
                  customClass: "text-3xl font-bold text-white mb-4"
                }
              },
              {
                id: "pageDescription",
                type: ComponentType.TEXT,
                text: "Join ongoing debates or start watching public discussions",
                style: { 
                  customClass: "text-lg text-gray-400"
                }
              }
            ]
          },

          // Search and Filter Section
          {
            id: "searchSection",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "mb-8"
            },
            children: [
              {
                id: "searchForm",
                type: ComponentType.FORM,
                name: "searchForm",
                style: { 
                  customClass: "flex gap-4 max-w-2xl mx-auto"
                },
                children: [
                  {
                    id: "searchInput",
                    type: ComponentType.INPUT,
                    text: "Search debates...",
                    name: "search",
                    style: { 
                      customClass: "flex-1 p-3 bg-gray-800 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    },
                    events: { onChange: "updateSearchQuery" }
                  },
                  {
                    id: "btnSearch",
                    type: ComponentType.BUTTON,
                    text: "🔍 Search",
                    style: { 
                      customClass: "px-6 py-3 bg-blue-600 hover:bg-blue-500 text-white font-medium rounded-lg transition-colors"
                    },
                    events: { onClick: "searchDebates" }
                  }
                ]
              }
            ]
          },

          // Loading State
          {
            id: "loadingState",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "hidden text-center py-12"
            },
            children: [
              {
                id: "loadingSpinner",
                type: ComponentType.TEXT,
                text: "🔄 Loading debates...",
                style: { 
                  customClass: "text-lg text-gray-400"
                }
              }
            ]
          },

          // Error State
          {
            id: "errorState",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "hidden bg-red-500/10 border border-red-500/30 rounded-lg p-6 text-center"
            },
            children: [
              {
                id: "errorMessage",
                type: ComponentType.TEXT,
                text: "Failed to load debates",
                style: { 
                  customClass: "text-red-300 font-medium"
                }
              }
            ]
          },

          // Debates Grid
          {
            id: "debatesGrid",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6 mb-8"
            },
            children: [
              // Sample Debate Cards (will be populated dynamically)
              {
                id: "debateCard1",
                type: ComponentType.CARD,
                style: { 
                  customClass: "bg-gray-800 border border-gray-700 hover:border-gray-600 rounded-lg overflow-hidden transition-all hover:transform hover:scale-105 cursor-pointer"
                },
                events: { onClick: "joinDebate" },
                children: [
                  {
                    id: "cardHeader1",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4 border-b border-gray-700"
                    },
                    children: [
                      {
                        id: "cardTitle1",
                        type: ComponentType.TEXT,
                        text: "Climate Change Solutions",
                        style: { 
                          customClass: "font-semibold text-white text-lg mb-2"
                        }
                      },
                      {
                        id: "cardMeta1",
                        type: ComponentType.TEXT,
                        text: "👥 12 participants • 🕒 Active",
                        style: { 
                          customClass: "text-sm text-gray-400"
                        }
                      }
                    ]
                  },
                  {
                    id: "cardContent1",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4"
                    },
                    children: [
                      {
                        id: "cardDescription1",
                        type: ComponentType.TEXT,
                        text: "Discussing various approaches to combat climate change and their effectiveness...",
                        style: { 
                          customClass: "text-gray-300 text-sm mb-3"
                        }
                      },
                      {
                        id: "cardTags1",
                        type: ComponentType.TEXT,
                        text: "#climate #environment #policy",
                        style: { 
                          customClass: "text-xs text-blue-400"
                        }
                      }
                    ]
                  }
                ]
              },

              {
                id: "debateCard2",
                type: ComponentType.CARD,
                style: { 
                  customClass: "bg-gray-800 border border-gray-700 hover:border-gray-600 rounded-lg overflow-hidden transition-all hover:transform hover:scale-105 cursor-pointer"
                },
                events: { onClick: "joinDebate" },
                children: [
                  {
                    id: "cardHeader2",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4 border-b border-gray-700"
                    },
                    children: [
                      {
                        id: "cardTitle2",
                        type: ComponentType.TEXT,
                        text: "AI Ethics & Society",
                        style: { 
                          customClass: "font-semibold text-white text-lg mb-2"
                        }
                      },
                      {
                        id: "cardMeta2",
                        type: ComponentType.TEXT,
                        text: "👥 8 participants • 🕒 Active",
                        style: { 
                          customClass: "text-sm text-gray-400"
                        }
                      }
                    ]
                  },
                  {
                    id: "cardContent2",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4"
                    },
                    children: [
                      {
                        id: "cardDescription2",
                        type: ComponentType.TEXT,
                        text: "Exploring the ethical implications of artificial intelligence in modern society...",
                        style: { 
                          customClass: "text-gray-300 text-sm mb-3"
                        }
                      },
                      {
                        id: "cardTags2",
                        type: ComponentType.TEXT,
                        text: "#ai #ethics #technology",
                        style: { 
                          customClass: "text-xs text-blue-400"
                        }
                      }
                    ]
                  }
                ]
              },

              {
                id: "debateCard3",
                type: ComponentType.CARD,
                style: { 
                  customClass: "bg-gray-800 border border-gray-700 hover:border-gray-600 rounded-lg overflow-hidden transition-all hover:transform hover:scale-105 cursor-pointer"
                },
                events: { onClick: "joinDebate" },
                children: [
                  {
                    id: "cardHeader3",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4 border-b border-gray-700"
                    },
                    children: [
                      {
                        id: "cardTitle3",
                        type: ComponentType.TEXT,
                        text: "Future of Education",
                        style: { 
                          customClass: "font-semibold text-white text-lg mb-2"
                        }
                      },
                      {
                        id: "cardMeta3",
                        type: ComponentType.TEXT,
                        text: "👥 15 participants • 🕒 Active",
                        style: { 
                          customClass: "text-sm text-gray-400"
                        }
                      }
                    ]
                  },
                  {
                    id: "cardContent3",
                    type: ComponentType.CONTAINER,
                    style: { 
                      customClass: "p-4"
                    },
                    children: [
                      {
                        id: "cardDescription3",
                        type: ComponentType.TEXT,
                        text: "How should education evolve to meet the needs of the 21st century?",
                        style: { 
                          customClass: "text-gray-300 text-sm mb-3"
                        }
                      },
                      {
                        id: "cardTags3",
                        type: ComponentType.TEXT,
                        text: "#education #future #society",
                        style: { 
                          customClass: "text-xs text-blue-400"
                        }
                      }
                    ]
                  }
                ]
              }
            ]
          },

          // Pagination
          {
            id: "pagination",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "flex justify-center items-center gap-2"
            },
            children: [
              {
                id: "btnPrevPage",
                type: ComponentType.BUTTON,
                text: "← Previous",
                style: { 
                  customClass: "px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors disabled:opacity-50"
                },
                events: { onClick: "previousPage" }
              },
              {
                id: "pageInfo",
                type: ComponentType.TEXT,
                text: "Page 1 of 5",
                style: { 
                  customClass: "px-4 py-2 text-gray-400"
                }
              },
              {
                id: "btnNextPage",
                type: ComponentType.BUTTON,
                text: "Next →",
                style: { 
                  customClass: "px-4 py-2 bg-gray-700 hover:bg-gray-600 text-white rounded-lg transition-colors"
                },
                events: { onClick: "nextPage" }
              }
            ]
          },

          // Empty State
          {
            id: "emptyState",
            type: ComponentType.CONTAINER,
            style: { 
              customClass: "hidden text-center py-12"
            },
            children: [
              {
                id: "emptyIcon",
                type: ComponentType.TEXT,
                text: "🔍",
                style: { 
                  customClass: "text-6xl mb-4"
                }
              },
              {
                id: "emptyTitle",
                type: ComponentType.TEXT,
                text: "No debates found",
                style: { 
                  customClass: "text-xl font-semibold text-gray-300 mb-2"
                }
              },
              {
                id: "emptyDescription",
                type: ComponentType.TEXT,
                text: "Try adjusting your search terms or browse all debates",
                style: { 
                  customClass: "text-gray-400"
                }
              }
            ]
          }
        ]
      }
    ]
  });
}
