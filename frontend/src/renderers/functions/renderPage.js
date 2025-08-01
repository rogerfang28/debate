// renderPage.jsx
export default function renderPage(data) {
  if (!data?.payload?.elements) {
    console.error("renderPage: invalid data format", data);
    return;
  }

  // Clear page
  document.body.innerHTML = "";

  // Action handler (replace with real logic later)
  function handleAction(action) {
    console.log(`Action triggered: ${action}`);
    if (action === "alertTest") alert("Test button clicked!");
  }

  // Loop through backend elements
  data.payload.elements.forEach(el => {
    let domEl;

    switch (el.type) {
      case "title":
        domEl = document.createElement("h1");
        domEl.textContent = el.props?.text || "";
        break;

      case "paragraph":
        domEl = document.createElement("p");
        domEl.textContent = el.props?.text || "";
        break;

      case "button":
        domEl = document.createElement("button");
        domEl.textContent = el.props?.text || "Button";
        domEl.onclick = () => handleAction(el.props?.action);
        break;

      case "list":
        domEl = document.createElement("ul");
        (el.props?.items || []).forEach(item => {
          const li = document.createElement("li");
          li.textContent = item.label || "";
          li.onclick = () => handleAction(item.action);
          domEl.appendChild(li);
        });
        break;

      case "form":
        domEl = document.createElement("form");
        (el.props?.fields || []).forEach(field => {
          const input = document.createElement("input");
          input.type = field.type || "text";
          input.name = field.name || "";
          input.placeholder = field.placeholder || "";
          domEl.appendChild(input);
        });
        const submitBtn = document.createElement("button");
        submitBtn.type = "submit";
        submitBtn.textContent = el.props?.submitText || "Submit";
        domEl.appendChild(submitBtn);
        domEl.onsubmit = (e) => {
          e.preventDefault();
          handleAction(el.props?.action);
        };
        break;

      case "nav":
        domEl = document.createElement("nav");
        const brand = document.createElement("span");
        brand.textContent = el.props?.brand || "";
        domEl.appendChild(brand);
        (el.props?.links || []).forEach(link => {
          const btn = document.createElement("button");
          btn.textContent = link.label || "";
          btn.onclick = () => handleAction(link.action);
          domEl.appendChild(btn);
        });
        break;

      default:
        console.warn(`Unknown element type: ${el.type}`);
        return;
    }

    document.body.appendChild(domEl);
  });
}
