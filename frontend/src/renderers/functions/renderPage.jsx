import React, { useState } from "react";
import { useNavigate } from "react-router-dom";
import { renderComponent } from "./componentRegistry.js";

export default function renderPage(data) {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({});
  
  if (!data?.payload?.layout) {
    console.error("renderPage: invalid data", data);
    return null;
  }

  // Get data from payload
  const pageData = data.payload.data || {};

  // Helper to resolve data fields
  const resolveDataField = (field, context = pageData) => {
    if (!field) return '';
    
    const parts = field.split('.');
    let value = context;
    
    for (const part of parts) {
      if (value && typeof value === 'object' && part in value) {
        value = value[part];
      } else {
        return '';
      }
    }
    
    return value;
  };

  // Helper to format data based on format type
  const formatData = (value, format) => {
    if (!format) return value;
    
    switch (format) {
      case 'date':
        return new Date(value).toLocaleDateString('en-US', {
          year: 'numeric',
          month: 'long',
          day: 'numeric',
          hour: '2-digit',
          minute: '2-digit'
        });
      case 'dateShort':
        return new Date(value).toLocaleDateString('en-US', {
          month: 'short',
          day: 'numeric'
        });
      default:
        return value;
    }
  };

  // Handle various actions
  const handleAction = (action, actionData = null, actionType = null) => {
    console.log(`Action triggered: ${action}`, { actionData, actionType });
    
    switch (action) {
      case 'logout':
        localStorage.removeItem('token');
        window.location.reload();
        break;
      case 'goHome':
        navigate('/');
        break;
      case 'goProfile':
        navigate('/profile');
        break;
      case 'goPublicDebates':
        navigate('/public-debates');
        break;
      case 'goLogin':
        navigate('/');
        break;
      case 'viewDebate':
        navigate(`/public-debate/${actionData}`);
        break;
      case 'enterRoom':
        navigate(`/graph/${actionData}`);
        break;
      case 'joinDebate':
        // Handle join debate logic
        console.log('Join debate:', actionData);
        break;
      case 'copyDebate':
        // Handle copy debate logic
        console.log('Copy debate:', actionData);
        break;
      case 'saveProfile':
        // Handle profile save
        console.log('Save profile:', formData);
        break;
      case 'searchDebates':
        // Handle search
        console.log('Search debates:', formData.search);
        break;
      case 'nextPage':
      case 'previousPage':
        // Handle pagination
        console.log('Pagination:', action);
        break;
      case 'openCreateModal':
      case 'openFriendsModal':
      case 'manageRoom':
        // Handle modal actions
        console.log('Modal action:', action, actionData);
        break;
      default:
        console.log('Unhandled action:', action);
    }
  };

  // Handle form input changes
  const handleInputChange = (name, value) => {
    setFormData(prev => ({
      ...prev,
      [name]: value
    }));
  };

  // Check conditions for conditional rendering
  const checkCondition = (condition, context = pageData) => {
    if (!condition) return true;
    
    // Handle simple existence checks
    if (condition.startsWith('!')) {
      const field = condition.substring(1);
      return !resolveDataField(field, context);
    }
    
    // Handle comparisons
    if (condition.includes('===')) {
      const [field, value] = condition.split('===').map(s => s.trim());
      return resolveDataField(field, context) === value;
    }
    
    if (condition.includes('>')) {
      const [field, value] = condition.split('>').map(s => s.trim());
      return resolveDataField(field, context) > parseInt(value);
    }
    
    // Simple field existence
    return !!resolveDataField(condition, context);
  };

  const renderElement = (el, key, context = pageData) => {
    // Skip if condition not met
    if (el.condition && !checkCondition(el.condition, context)) {
      return null;
    }

    // Handle registered components
    if (el.type === "component") {
      return renderComponent(el.name, el.props || {}, key);
    }

    // Handle conditional rendering
    if (el.type === "conditional") {
      if (checkCondition(el.condition, context)) {
        return (
          <div key={key}>
            {el.children?.map((child, idx) => renderElement(child, idx, context))}
          </div>
        );
      }
      return null;
    }

    // Handle repeat/loop rendering
    if (el.type === "repeat") {
      const dataSource = resolveDataField(el.dataSource, context);
      if (!Array.isArray(dataSource)) return null;
      
      return (
        <div key={key}>
          {dataSource.map((item, idx) => 
            renderElement(el.template, `${key}-${idx}`, item)
          )}
        </div>
      );
    }

    // Handle forms
    if (el.type === "form") {
      return (
        <form 
          key={key} 
          className={el.className || ""}
          onSubmit={(e) => {
            e.preventDefault();
            if (el.action) {
              handleAction(el.action, formData, 'submit');
            }
          }}
        >
          {el.children?.map((child, idx) => renderElement(child, idx, context))}
        </form>
      );
    }

    // Handle inputs
    if (el.type === "input") {
      const dataValue = el.dataField ? resolveDataField(el.dataField, context) : '';
      const currentValue = formData[el.name] !== undefined ? formData[el.name] : dataValue;
      
      return (
        <input
          key={key}
          type={el.inputType || "text"}
          name={el.name}
          id={el.id}
          className={el.className || ""}
          placeholder={el.placeholder}
          value={currentValue}
          disabled={el.disabled}
          required={el.required}
          onChange={(e) => handleInputChange(el.name, e.target.value)}
        />
      );
    }

    // Handle buttons
    if (el.type === "button") {
      const handleClick = () => {
        if (el.confirm && !window.confirm(el.confirm)) {
          return;
        }
        
        let actionData = el.actionData;
        if (actionData && typeof actionData === 'string') {
          actionData = resolveDataField(actionData, context);
        }
        
        handleAction(el.action, actionData, el.actionType);
      };

      return (
        <button
          key={key}
          type={el.actionType === 'submit' ? 'submit' : 'button'}
          className={el.className || ""}
          onClick={el.actionType !== 'submit' ? handleClick : undefined}
        >
          {el.text}
        </button>
      );
    }

    // Handle labels
    if (el.type === "label") {
      return (
        <label key={key} className={el.className || ""} htmlFor={el.for}>
          {el.text}
        </label>
      );
    }

    // Generic HTML elements
    const Tag = el.type || "div";
    
    // Build text content
    let textContent = el.text || '';
    
    // Handle data field binding
    if (el.dataField) {
      textContent = resolveDataField(el.dataField, context);
      if (el.format) {
        textContent = formatData(textContent, el.format);
      }
    }
    
    // Handle data appending
    if (el.dataAppend) {
      const appendValue = resolveDataField(el.dataAppend, context);
      textContent += (el.format ? formatData(appendValue, el.format) : appendValue);
    }
    if (el.dataAppend2) {
      textContent += el.dataAppend2;
    }
    if (el.dataAppend3) {
      const appendValue = resolveDataField(el.dataAppend3, context);
      textContent += appendValue;
    }

    // Handle click actions
    const clickProps = {};
    if (el.action) {
      clickProps.onClick = () => {
        let actionData = el.actionData;
        if (actionData && typeof actionData === 'string') {
          actionData = resolveDataField(actionData, context);
        }
        handleAction(el.action, actionData);
      };
      clickProps.style = { cursor: 'pointer', ...clickProps.style };
    }

    return (
      <Tag key={key} className={el.className || ""} {...clickProps}>
        {textContent}
        {el.children?.map((child, idx) => renderElement(child, idx, context))}
      </Tag>
    );
  };

  return renderElement(data.payload.layout);
}
