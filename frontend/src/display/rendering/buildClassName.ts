// TypeScript interface for component style
export interface ComponentStyle {
  customClass?: string;
  padding?: string;
  margin?: string;
  bgColor?: string;
  bg_color?: string;
  textColor?: string;
  text_color?: string;
  border?: string;
  rounded?: string;
  shadow?: string;
  gap?: string;
  [key: string]: any;
}

export default function buildClassName(style?: ComponentStyle): string {
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
