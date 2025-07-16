Graph JSON format:
{
  "nodes": {
    "node-1": {
      "label": "Climate change is real",
      "x": 100,
      "y": 200,
      "metadata": {
        "createdBy": "user123",
        "createdAt": "2025-07-16T08:00:00Z"
      }
    },
    "node-2": {
      "label": "CO2 causes warming",
      "x": 300,
      "y": 250,
      "metadata": {}
    }
  },
  "edges": [
    {
      "source": "node-1",
      "target": "node-2",
      "metadata": {
        "relation": "supports",
        "createdBy": "user123",
        "createdAt": "2025-07-16T08:10:00Z"
      }
    }
  ]
}

