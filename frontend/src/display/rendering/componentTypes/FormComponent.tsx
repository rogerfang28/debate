import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";

interface FormComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    attributes?: { [key: string]: any };
  };
}

const FormComponent: React.FC<FormComponentProps> = ({ component, className, style, events }) => {
  return (
    <form className={className} style={style} {...events} {...component.attributes}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </form>
  );
};

export default FormComponent;
