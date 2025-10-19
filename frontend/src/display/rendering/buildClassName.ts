// TypeScript interface for component style
// Note: protobuf codegen converts custom_class -> customClass
export interface ComponentStyle {
  customClass?: string;
  [key: string]: any;
}

export default function buildClassName(style?: ComponentStyle): string {
  return style?.customClass || "";
}
