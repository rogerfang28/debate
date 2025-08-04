export default function buildClassName(style) {
  return [
    style?.customClass || "",
    style?.padding || "",
    style?.margin || "",
    style?.bgColor || style?.bg_color || "",
    style?.textColor || style?.text_color || "",
    style?.border || "",
    style?.rounded || "",
    style?.shadow || "",
    style?.gap || "",
  ]
    .filter(Boolean)
    .join(" ");
}
