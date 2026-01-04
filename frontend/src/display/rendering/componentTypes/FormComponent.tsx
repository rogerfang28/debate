import React from "react";
import ComponentRenderer from "../ComponentRenderer";
import { BaseComponentProps } from "./TextComponent";
import handleEvent from "../../events/handleEvent";

interface FormComponentProps extends BaseComponentProps {
  component: BaseComponentProps['component'] & {
    attributes?: { [key: string]: any };
  };
}

const FormComponent: React.FC<FormComponentProps> = ({ component, className, style }) => {
  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    console.log(`📋 Form "${component.id}" submitted`);
    
    // Send event to backend on form submit
    handleEvent(
      e as any,
      component,
      'onSubmit',
      component.id || 'unknown-form'
    );
  };

  return (
    <form id={component.id} className={className} style={style} onSubmit={handleSubmit} {...component.attributes}>
      {component.children?.map((child, idx: number) => (
        <ComponentRenderer key={child.id || idx} component={child} />
      ))}
    </form>
  );
};

export default FormComponent;
