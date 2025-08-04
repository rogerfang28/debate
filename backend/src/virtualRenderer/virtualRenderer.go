package main

import (
    "log"
    "os"

    pb "debate/go/protos" // 👈 replace with the import path that matches your go.mod
    "google.golang.org/protobuf/proto"
)

func main() {
    // Build your Page object
    page := &pb.Page{
        PageId: "home",
        Title:  "Debate Home Page",
        Components: []*pb.Component{
            {
                Id:        "btn1",
                Type:      pb.ComponentType_BUTTON,
                Text:      "Add Node",
                ClassName: "bg-blue-500 text-white p-2 rounded",
                Events: map[string]string{
                    "onClick": "addNode",
                },
            },
        },
    }

    // Serialize to protobuf binary
    data, err := proto.Marshal(page)
    if err != nil {
        log.Fatal("Failed to serialize page:", err)
    }

    // Save as .pb file
    if err := os.WriteFile("home_page.pb", data, 0644); err != nil {
        log.Fatal("Failed to write file:", err)
    }

    log.Println("✅ Saved Protobuf to home_page.pb")
}