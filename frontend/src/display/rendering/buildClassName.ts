// TypeScript interface for component style
// Note: protobuf codegen converts custom_class -> customClass
export interface ComponentStyle {
  customClass?: string;
  [key: string]: any;
}

export default function buildClassName(style?: ComponentStyle): string {
  return style?.customClass || "";
}

// The server's class strings mix layout (w-full, flex-1, self-start, mt-4)
// with paint (bg-*, text-*, border-*, rounded, p*-, font-*). Components that
// paint themselves from the theme -- buttons and fields -- keep only the
// layout, so the theme's look is not fought by a stray bg-green-600.
const PAINT = /^(?:[a-z-]+:)*(?:bg-|text-|border|rounded|p[xytrbl]?-|font-|shadow|transition|ring|placeholder-|underline|from-|to-|tracking-|leading-)/;

export function layoutClasses(className?: string): string {
  return (className || "")
    .split(/\s+/)
    .filter((c) => c && !PAINT.test(c))
    .join(" ");
}
