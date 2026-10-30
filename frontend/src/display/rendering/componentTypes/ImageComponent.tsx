import React from "react";
import { BaseComponentProps } from "./TextComponent";

// A picture by its web address (attribute src). Pictures come from other
// sites -- a source's screenshot is hosted wherever its author put it -- so
// no referrer is sent, loading waits until it is on screen, and one that
// fails to load disappears rather than showing a broken icon. With
// data-href it is a link, opened in a new tab like a source link.
const ImageComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const [failed, setFailed] = React.useState(false);
  const attrs: Record<string, string> = (component as any).attributes || {};
  const src = attrs["src"];
  if (failed || !src || !/^https?:\/\//i.test(src)) return null;

  const img = (
    <img
      id={component.id}
      src={src}
      alt={component.text || ""}
      className={className}
      style={style}
      loading="lazy"
      decoding="async"
      referrerPolicy="no-referrer"
      onError={() => setFailed(true)}
    />
  );

  const href: string | undefined = attrs["data-href"];
  if (!href || !/^https?:\/\//i.test(href)) return img;
  return (
    <a href={href} target="_blank" rel="noopener noreferrer" onClick={(e) => e.stopPropagation()}>
      {img}
    </a>
  );
};

export default ImageComponent;
