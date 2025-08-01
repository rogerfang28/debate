// renderPage.js
export default function renderPage(data) {
  if (!data || typeof data !== "object") {
    console.error("renderPage: invalid data", data);
    return;
  }

  // Use the payload part of the data
  const { payload } = data;
  if (!payload) {
    console.error("renderPage: no payload found in data", data);
    return;
  }

  // Clear the whole page
  document.body.innerHTML = "";

  // Title
  if (payload.title) {
    const titleEl = document.createElement("h1");
    titleEl.textContent = payload.title;
    document.body.appendChild(titleEl);
  }

  // Description
  if (payload.description) {
    const descEl = document.createElement("p");
    descEl.textContent = payload.description;
    document.body.appendChild(descEl);
  }

  // Items list
  if (Array.isArray(payload.items) && payload.items.length > 0) {
    const listEl = document.createElement("ul");
    payload.items.forEach(item => {
      const li = document.createElement("li");
      li.textContent = item.name || `Item ${item.id}`;
      listEl.appendChild(li);
    });
    document.body.appendChild(listEl);
  }
}
