/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
import * as $protobuf from "protobufjs/minimal.js";

// Common aliases
const $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
const $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

export const debate = $root.debate = (() => {

    /**
     * Namespace debate.
     * @exports debate
     * @namespace
     */
    const debate = {};

    debate.Page = (function() {

        /**
         * Properties of a Page.
         * @memberof debate
         * @interface IPage
         * @property {string|null} [pageId] Page pageId
         * @property {string|null} [title] Page title
         * @property {Array.<debate.IComponent>|null} [components] Page components
         * @property {debate.ITheme|null} [theme] Page theme
         */

        /**
         * Constructs a new Page.
         * @memberof debate
         * @classdesc Represents a Page.
         * @implements IPage
         * @constructor
         * @param {debate.IPage=} [properties] Properties to set
         */
        function Page(properties) {
            this.components = [];
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Page pageId.
         * @member {string} pageId
         * @memberof debate.Page
         * @instance
         */
        Page.prototype.pageId = "";

        /**
         * Page title.
         * @member {string} title
         * @memberof debate.Page
         * @instance
         */
        Page.prototype.title = "";

        /**
         * Page components.
         * @member {Array.<debate.IComponent>} components
         * @memberof debate.Page
         * @instance
         */
        Page.prototype.components = $util.emptyArray;

        /**
         * Page theme.
         * @member {debate.ITheme|null|undefined} theme
         * @memberof debate.Page
         * @instance
         */
        Page.prototype.theme = null;

        /**
         * Creates a new Page instance using the specified properties.
         * @function create
         * @memberof debate.Page
         * @static
         * @param {debate.IPage=} [properties] Properties to set
         * @returns {debate.Page} Page instance
         */
        Page.create = function create(properties) {
            return new Page(properties);
        };

        /**
         * Encodes the specified Page message. Does not implicitly {@link debate.Page.verify|verify} messages.
         * @function encode
         * @memberof debate.Page
         * @static
         * @param {debate.IPage} message Page message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Page.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.pageId != null && Object.hasOwnProperty.call(message, "pageId"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.pageId);
            if (message.title != null && Object.hasOwnProperty.call(message, "title"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.title);
            if (message.components != null && message.components.length)
                for (let i = 0; i < message.components.length; ++i)
                    $root.debate.Component.encode(message.components[i], writer.uint32(/* id 3, wireType 2 =*/26).fork()).ldelim();
            if (message.theme != null && Object.hasOwnProperty.call(message, "theme"))
                $root.debate.Theme.encode(message.theme, writer.uint32(/* id 4, wireType 2 =*/34).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified Page message, length delimited. Does not implicitly {@link debate.Page.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Page
         * @static
         * @param {debate.IPage} message Page message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Page.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Page message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Page
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Page} Page
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Page.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Page();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.pageId = reader.string();
                        break;
                    }
                case 2: {
                        message.title = reader.string();
                        break;
                    }
                case 3: {
                        if (!(message.components && message.components.length))
                            message.components = [];
                        message.components.push($root.debate.Component.decode(reader, reader.uint32()));
                        break;
                    }
                case 4: {
                        message.theme = $root.debate.Theme.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Page message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Page
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Page} Page
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Page.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Page message.
         * @function verify
         * @memberof debate.Page
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Page.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.pageId != null && message.hasOwnProperty("pageId"))
                if (!$util.isString(message.pageId))
                    return "pageId: string expected";
            if (message.title != null && message.hasOwnProperty("title"))
                if (!$util.isString(message.title))
                    return "title: string expected";
            if (message.components != null && message.hasOwnProperty("components")) {
                if (!Array.isArray(message.components))
                    return "components: array expected";
                for (let i = 0; i < message.components.length; ++i) {
                    let error = $root.debate.Component.verify(message.components[i]);
                    if (error)
                        return "components." + error;
                }
            }
            if (message.theme != null && message.hasOwnProperty("theme")) {
                let error = $root.debate.Theme.verify(message.theme);
                if (error)
                    return "theme." + error;
            }
            return null;
        };

        /**
         * Creates a Page message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Page
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Page} Page
         */
        Page.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Page)
                return object;
            let message = new $root.debate.Page();
            if (object.pageId != null)
                message.pageId = String(object.pageId);
            if (object.title != null)
                message.title = String(object.title);
            if (object.components) {
                if (!Array.isArray(object.components))
                    throw TypeError(".debate.Page.components: array expected");
                message.components = [];
                for (let i = 0; i < object.components.length; ++i) {
                    if (typeof object.components[i] !== "object")
                        throw TypeError(".debate.Page.components: object expected");
                    message.components[i] = $root.debate.Component.fromObject(object.components[i]);
                }
            }
            if (object.theme != null) {
                if (typeof object.theme !== "object")
                    throw TypeError(".debate.Page.theme: object expected");
                message.theme = $root.debate.Theme.fromObject(object.theme);
            }
            return message;
        };

        /**
         * Creates a plain object from a Page message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Page
         * @static
         * @param {debate.Page} message Page
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Page.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.arrays || options.defaults)
                object.components = [];
            if (options.defaults) {
                object.pageId = "";
                object.title = "";
                object.theme = null;
            }
            if (message.pageId != null && message.hasOwnProperty("pageId"))
                object.pageId = message.pageId;
            if (message.title != null && message.hasOwnProperty("title"))
                object.title = message.title;
            if (message.components && message.components.length) {
                object.components = [];
                for (let j = 0; j < message.components.length; ++j)
                    object.components[j] = $root.debate.Component.toObject(message.components[j], options);
            }
            if (message.theme != null && message.hasOwnProperty("theme"))
                object.theme = $root.debate.Theme.toObject(message.theme, options);
            return object;
        };

        /**
         * Converts this Page to JSON.
         * @function toJSON
         * @memberof debate.Page
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Page.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Page
         * @function getTypeUrl
         * @memberof debate.Page
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Page.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Page";
        };

        return Page;
    })();

    debate.Component = (function() {

        /**
         * Properties of a Component.
         * @memberof debate
         * @interface IComponent
         * @property {string|null} [id] Component id
         * @property {debate.ComponentType|null} [type] Component type
         * @property {string|null} [text] Component text
         * @property {Array.<debate.IComponent>|null} [children] Component children
         * @property {debate.IStyle|null} [style] Component style
         * @property {Object.<string,string>|null} [events] Component events
         */

        /**
         * Constructs a new Component.
         * @memberof debate
         * @classdesc Represents a Component.
         * @implements IComponent
         * @constructor
         * @param {debate.IComponent=} [properties] Properties to set
         */
        function Component(properties) {
            this.children = [];
            this.events = {};
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Component id.
         * @member {string} id
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.id = "";

        /**
         * Component type.
         * @member {debate.ComponentType} type
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.type = 0;

        /**
         * Component text.
         * @member {string} text
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.text = "";

        /**
         * Component children.
         * @member {Array.<debate.IComponent>} children
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.children = $util.emptyArray;

        /**
         * Component style.
         * @member {debate.IStyle|null|undefined} style
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.style = null;

        /**
         * Component events.
         * @member {Object.<string,string>} events
         * @memberof debate.Component
         * @instance
         */
        Component.prototype.events = $util.emptyObject;

        /**
         * Creates a new Component instance using the specified properties.
         * @function create
         * @memberof debate.Component
         * @static
         * @param {debate.IComponent=} [properties] Properties to set
         * @returns {debate.Component} Component instance
         */
        Component.create = function create(properties) {
            return new Component(properties);
        };

        /**
         * Encodes the specified Component message. Does not implicitly {@link debate.Component.verify|verify} messages.
         * @function encode
         * @memberof debate.Component
         * @static
         * @param {debate.IComponent} message Component message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Component.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.id != null && Object.hasOwnProperty.call(message, "id"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.id);
            if (message.type != null && Object.hasOwnProperty.call(message, "type"))
                writer.uint32(/* id 2, wireType 0 =*/16).int32(message.type);
            if (message.text != null && Object.hasOwnProperty.call(message, "text"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.text);
            if (message.children != null && message.children.length)
                for (let i = 0; i < message.children.length; ++i)
                    $root.debate.Component.encode(message.children[i], writer.uint32(/* id 4, wireType 2 =*/34).fork()).ldelim();
            if (message.style != null && Object.hasOwnProperty.call(message, "style"))
                $root.debate.Style.encode(message.style, writer.uint32(/* id 5, wireType 2 =*/42).fork()).ldelim();
            if (message.events != null && Object.hasOwnProperty.call(message, "events"))
                for (let keys = Object.keys(message.events), i = 0; i < keys.length; ++i)
                    writer.uint32(/* id 6, wireType 2 =*/50).fork().uint32(/* id 1, wireType 2 =*/10).string(keys[i]).uint32(/* id 2, wireType 2 =*/18).string(message.events[keys[i]]).ldelim();
            return writer;
        };

        /**
         * Encodes the specified Component message, length delimited. Does not implicitly {@link debate.Component.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Component
         * @static
         * @param {debate.IComponent} message Component message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Component.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Component message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Component
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Component} Component
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Component.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Component(), key, value;
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.id = reader.string();
                        break;
                    }
                case 2: {
                        message.type = reader.int32();
                        break;
                    }
                case 3: {
                        message.text = reader.string();
                        break;
                    }
                case 4: {
                        if (!(message.children && message.children.length))
                            message.children = [];
                        message.children.push($root.debate.Component.decode(reader, reader.uint32()));
                        break;
                    }
                case 5: {
                        message.style = $root.debate.Style.decode(reader, reader.uint32());
                        break;
                    }
                case 6: {
                        if (message.events === $util.emptyObject)
                            message.events = {};
                        let end2 = reader.uint32() + reader.pos;
                        key = "";
                        value = "";
                        while (reader.pos < end2) {
                            let tag2 = reader.uint32();
                            switch (tag2 >>> 3) {
                            case 1:
                                key = reader.string();
                                break;
                            case 2:
                                value = reader.string();
                                break;
                            default:
                                reader.skipType(tag2 & 7);
                                break;
                            }
                        }
                        message.events[key] = value;
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Component message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Component
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Component} Component
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Component.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Component message.
         * @function verify
         * @memberof debate.Component
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Component.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.id != null && message.hasOwnProperty("id"))
                if (!$util.isString(message.id))
                    return "id: string expected";
            if (message.type != null && message.hasOwnProperty("type"))
                switch (message.type) {
                default:
                    return "type: enum value expected";
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    break;
                }
            if (message.text != null && message.hasOwnProperty("text"))
                if (!$util.isString(message.text))
                    return "text: string expected";
            if (message.children != null && message.hasOwnProperty("children")) {
                if (!Array.isArray(message.children))
                    return "children: array expected";
                for (let i = 0; i < message.children.length; ++i) {
                    let error = $root.debate.Component.verify(message.children[i]);
                    if (error)
                        return "children." + error;
                }
            }
            if (message.style != null && message.hasOwnProperty("style")) {
                let error = $root.debate.Style.verify(message.style);
                if (error)
                    return "style." + error;
            }
            if (message.events != null && message.hasOwnProperty("events")) {
                if (!$util.isObject(message.events))
                    return "events: object expected";
                let key = Object.keys(message.events);
                for (let i = 0; i < key.length; ++i)
                    if (!$util.isString(message.events[key[i]]))
                        return "events: string{k:string} expected";
            }
            return null;
        };

        /**
         * Creates a Component message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Component
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Component} Component
         */
        Component.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Component)
                return object;
            let message = new $root.debate.Component();
            if (object.id != null)
                message.id = String(object.id);
            switch (object.type) {
            default:
                if (typeof object.type === "number") {
                    message.type = object.type;
                    break;
                }
                break;
            case "UNKNOWN":
            case 0:
                message.type = 0;
                break;
            case "TEXT":
            case 1:
                message.type = 1;
                break;
            case "BUTTON":
            case 2:
                message.type = 2;
                break;
            case "INPUT":
            case 3:
                message.type = 3;
                break;
            case "NODE_GRAPH":
            case 4:
                message.type = 4;
                break;
            case "CHAT":
            case 5:
                message.type = 5;
                break;
            case "IMAGE":
            case 6:
                message.type = 6;
                break;
            case "LIST":
            case 7:
                message.type = 7;
                break;
            case "CONTAINER":
            case 8:
                message.type = 8;
                break;
            case "MODAL":
            case 9:
                message.type = 9;
                break;
            }
            if (object.text != null)
                message.text = String(object.text);
            if (object.children) {
                if (!Array.isArray(object.children))
                    throw TypeError(".debate.Component.children: array expected");
                message.children = [];
                for (let i = 0; i < object.children.length; ++i) {
                    if (typeof object.children[i] !== "object")
                        throw TypeError(".debate.Component.children: object expected");
                    message.children[i] = $root.debate.Component.fromObject(object.children[i]);
                }
            }
            if (object.style != null) {
                if (typeof object.style !== "object")
                    throw TypeError(".debate.Component.style: object expected");
                message.style = $root.debate.Style.fromObject(object.style);
            }
            if (object.events) {
                if (typeof object.events !== "object")
                    throw TypeError(".debate.Component.events: object expected");
                message.events = {};
                for (let keys = Object.keys(object.events), i = 0; i < keys.length; ++i)
                    message.events[keys[i]] = String(object.events[keys[i]]);
            }
            return message;
        };

        /**
         * Creates a plain object from a Component message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Component
         * @static
         * @param {debate.Component} message Component
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Component.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.arrays || options.defaults)
                object.children = [];
            if (options.objects || options.defaults)
                object.events = {};
            if (options.defaults) {
                object.id = "";
                object.type = options.enums === String ? "UNKNOWN" : 0;
                object.text = "";
                object.style = null;
            }
            if (message.id != null && message.hasOwnProperty("id"))
                object.id = message.id;
            if (message.type != null && message.hasOwnProperty("type"))
                object.type = options.enums === String ? $root.debate.ComponentType[message.type] === undefined ? message.type : $root.debate.ComponentType[message.type] : message.type;
            if (message.text != null && message.hasOwnProperty("text"))
                object.text = message.text;
            if (message.children && message.children.length) {
                object.children = [];
                for (let j = 0; j < message.children.length; ++j)
                    object.children[j] = $root.debate.Component.toObject(message.children[j], options);
            }
            if (message.style != null && message.hasOwnProperty("style"))
                object.style = $root.debate.Style.toObject(message.style, options);
            let keys2;
            if (message.events && (keys2 = Object.keys(message.events)).length) {
                object.events = {};
                for (let j = 0; j < keys2.length; ++j)
                    object.events[keys2[j]] = message.events[keys2[j]];
            }
            return object;
        };

        /**
         * Converts this Component to JSON.
         * @function toJSON
         * @memberof debate.Component
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Component.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Component
         * @function getTypeUrl
         * @memberof debate.Component
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Component.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Component";
        };

        return Component;
    })();

    /**
     * ComponentType enum.
     * @name debate.ComponentType
     * @enum {number}
     * @property {number} UNKNOWN=0 UNKNOWN value
     * @property {number} TEXT=1 TEXT value
     * @property {number} BUTTON=2 BUTTON value
     * @property {number} INPUT=3 INPUT value
     * @property {number} NODE_GRAPH=4 NODE_GRAPH value
     * @property {number} CHAT=5 CHAT value
     * @property {number} IMAGE=6 IMAGE value
     * @property {number} LIST=7 LIST value
     * @property {number} CONTAINER=8 CONTAINER value
     * @property {number} MODAL=9 MODAL value
     */
    debate.ComponentType = (function() {
        const valuesById = {}, values = Object.create(valuesById);
        values[valuesById[0] = "UNKNOWN"] = 0;
        values[valuesById[1] = "TEXT"] = 1;
        values[valuesById[2] = "BUTTON"] = 2;
        values[valuesById[3] = "INPUT"] = 3;
        values[valuesById[4] = "NODE_GRAPH"] = 4;
        values[valuesById[5] = "CHAT"] = 5;
        values[valuesById[6] = "IMAGE"] = 6;
        values[valuesById[7] = "LIST"] = 7;
        values[valuesById[8] = "CONTAINER"] = 8;
        values[valuesById[9] = "MODAL"] = 9;
        return values;
    })();

    debate.Style = (function() {

        /**
         * Properties of a Style.
         * @memberof debate
         * @interface IStyle
         * @property {debate.LayoutType|null} [layout] Style layout
         * @property {debate.IPosition|null} [position] Style position
         * @property {debate.ISize|null} [size] Style size
         * @property {string|null} [padding] Style padding
         * @property {string|null} [margin] Style margin
         * @property {string|null} [bgColor] Style bgColor
         * @property {string|null} [textColor] Style textColor
         * @property {string|null} [border] Style border
         * @property {string|null} [rounded] Style rounded
         * @property {string|null} [shadow] Style shadow
         * @property {string|null} [gap] Style gap
         * @property {boolean|null} [scrollable] Style scrollable
         * @property {string|null} [customClass] Style customClass
         */

        /**
         * Constructs a new Style.
         * @memberof debate
         * @classdesc Represents a Style.
         * @implements IStyle
         * @constructor
         * @param {debate.IStyle=} [properties] Properties to set
         */
        function Style(properties) {
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Style layout.
         * @member {debate.LayoutType} layout
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.layout = 0;

        /**
         * Style position.
         * @member {debate.IPosition|null|undefined} position
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.position = null;

        /**
         * Style size.
         * @member {debate.ISize|null|undefined} size
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.size = null;

        /**
         * Style padding.
         * @member {string} padding
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.padding = "";

        /**
         * Style margin.
         * @member {string} margin
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.margin = "";

        /**
         * Style bgColor.
         * @member {string} bgColor
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.bgColor = "";

        /**
         * Style textColor.
         * @member {string} textColor
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.textColor = "";

        /**
         * Style border.
         * @member {string} border
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.border = "";

        /**
         * Style rounded.
         * @member {string} rounded
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.rounded = "";

        /**
         * Style shadow.
         * @member {string} shadow
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.shadow = "";

        /**
         * Style gap.
         * @member {string} gap
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.gap = "";

        /**
         * Style scrollable.
         * @member {boolean} scrollable
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.scrollable = false;

        /**
         * Style customClass.
         * @member {string} customClass
         * @memberof debate.Style
         * @instance
         */
        Style.prototype.customClass = "";

        /**
         * Creates a new Style instance using the specified properties.
         * @function create
         * @memberof debate.Style
         * @static
         * @param {debate.IStyle=} [properties] Properties to set
         * @returns {debate.Style} Style instance
         */
        Style.create = function create(properties) {
            return new Style(properties);
        };

        /**
         * Encodes the specified Style message. Does not implicitly {@link debate.Style.verify|verify} messages.
         * @function encode
         * @memberof debate.Style
         * @static
         * @param {debate.IStyle} message Style message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Style.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.layout != null && Object.hasOwnProperty.call(message, "layout"))
                writer.uint32(/* id 1, wireType 0 =*/8).int32(message.layout);
            if (message.position != null && Object.hasOwnProperty.call(message, "position"))
                $root.debate.Position.encode(message.position, writer.uint32(/* id 2, wireType 2 =*/18).fork()).ldelim();
            if (message.size != null && Object.hasOwnProperty.call(message, "size"))
                $root.debate.Size.encode(message.size, writer.uint32(/* id 3, wireType 2 =*/26).fork()).ldelim();
            if (message.padding != null && Object.hasOwnProperty.call(message, "padding"))
                writer.uint32(/* id 4, wireType 2 =*/34).string(message.padding);
            if (message.margin != null && Object.hasOwnProperty.call(message, "margin"))
                writer.uint32(/* id 5, wireType 2 =*/42).string(message.margin);
            if (message.bgColor != null && Object.hasOwnProperty.call(message, "bgColor"))
                writer.uint32(/* id 6, wireType 2 =*/50).string(message.bgColor);
            if (message.textColor != null && Object.hasOwnProperty.call(message, "textColor"))
                writer.uint32(/* id 7, wireType 2 =*/58).string(message.textColor);
            if (message.border != null && Object.hasOwnProperty.call(message, "border"))
                writer.uint32(/* id 8, wireType 2 =*/66).string(message.border);
            if (message.rounded != null && Object.hasOwnProperty.call(message, "rounded"))
                writer.uint32(/* id 9, wireType 2 =*/74).string(message.rounded);
            if (message.shadow != null && Object.hasOwnProperty.call(message, "shadow"))
                writer.uint32(/* id 10, wireType 2 =*/82).string(message.shadow);
            if (message.gap != null && Object.hasOwnProperty.call(message, "gap"))
                writer.uint32(/* id 11, wireType 2 =*/90).string(message.gap);
            if (message.scrollable != null && Object.hasOwnProperty.call(message, "scrollable"))
                writer.uint32(/* id 12, wireType 0 =*/96).bool(message.scrollable);
            if (message.customClass != null && Object.hasOwnProperty.call(message, "customClass"))
                writer.uint32(/* id 13, wireType 2 =*/106).string(message.customClass);
            return writer;
        };

        /**
         * Encodes the specified Style message, length delimited. Does not implicitly {@link debate.Style.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Style
         * @static
         * @param {debate.IStyle} message Style message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Style.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Style message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Style
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Style} Style
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Style.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Style();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.layout = reader.int32();
                        break;
                    }
                case 2: {
                        message.position = $root.debate.Position.decode(reader, reader.uint32());
                        break;
                    }
                case 3: {
                        message.size = $root.debate.Size.decode(reader, reader.uint32());
                        break;
                    }
                case 4: {
                        message.padding = reader.string();
                        break;
                    }
                case 5: {
                        message.margin = reader.string();
                        break;
                    }
                case 6: {
                        message.bgColor = reader.string();
                        break;
                    }
                case 7: {
                        message.textColor = reader.string();
                        break;
                    }
                case 8: {
                        message.border = reader.string();
                        break;
                    }
                case 9: {
                        message.rounded = reader.string();
                        break;
                    }
                case 10: {
                        message.shadow = reader.string();
                        break;
                    }
                case 11: {
                        message.gap = reader.string();
                        break;
                    }
                case 12: {
                        message.scrollable = reader.bool();
                        break;
                    }
                case 13: {
                        message.customClass = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Style message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Style
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Style} Style
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Style.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Style message.
         * @function verify
         * @memberof debate.Style
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Style.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.layout != null && message.hasOwnProperty("layout"))
                switch (message.layout) {
                default:
                    return "layout: enum value expected";
                case 0:
                case 1:
                case 2:
                case 3:
                    break;
                }
            if (message.position != null && message.hasOwnProperty("position")) {
                let error = $root.debate.Position.verify(message.position);
                if (error)
                    return "position." + error;
            }
            if (message.size != null && message.hasOwnProperty("size")) {
                let error = $root.debate.Size.verify(message.size);
                if (error)
                    return "size." + error;
            }
            if (message.padding != null && message.hasOwnProperty("padding"))
                if (!$util.isString(message.padding))
                    return "padding: string expected";
            if (message.margin != null && message.hasOwnProperty("margin"))
                if (!$util.isString(message.margin))
                    return "margin: string expected";
            if (message.bgColor != null && message.hasOwnProperty("bgColor"))
                if (!$util.isString(message.bgColor))
                    return "bgColor: string expected";
            if (message.textColor != null && message.hasOwnProperty("textColor"))
                if (!$util.isString(message.textColor))
                    return "textColor: string expected";
            if (message.border != null && message.hasOwnProperty("border"))
                if (!$util.isString(message.border))
                    return "border: string expected";
            if (message.rounded != null && message.hasOwnProperty("rounded"))
                if (!$util.isString(message.rounded))
                    return "rounded: string expected";
            if (message.shadow != null && message.hasOwnProperty("shadow"))
                if (!$util.isString(message.shadow))
                    return "shadow: string expected";
            if (message.gap != null && message.hasOwnProperty("gap"))
                if (!$util.isString(message.gap))
                    return "gap: string expected";
            if (message.scrollable != null && message.hasOwnProperty("scrollable"))
                if (typeof message.scrollable !== "boolean")
                    return "scrollable: boolean expected";
            if (message.customClass != null && message.hasOwnProperty("customClass"))
                if (!$util.isString(message.customClass))
                    return "customClass: string expected";
            return null;
        };

        /**
         * Creates a Style message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Style
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Style} Style
         */
        Style.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Style)
                return object;
            let message = new $root.debate.Style();
            switch (object.layout) {
            default:
                if (typeof object.layout === "number") {
                    message.layout = object.layout;
                    break;
                }
                break;
            case "ABSOLUTE":
            case 0:
                message.layout = 0;
                break;
            case "FLEX_ROW":
            case 1:
                message.layout = 1;
                break;
            case "FLEX_COL":
            case 2:
                message.layout = 2;
                break;
            case "GRID":
            case 3:
                message.layout = 3;
                break;
            }
            if (object.position != null) {
                if (typeof object.position !== "object")
                    throw TypeError(".debate.Style.position: object expected");
                message.position = $root.debate.Position.fromObject(object.position);
            }
            if (object.size != null) {
                if (typeof object.size !== "object")
                    throw TypeError(".debate.Style.size: object expected");
                message.size = $root.debate.Size.fromObject(object.size);
            }
            if (object.padding != null)
                message.padding = String(object.padding);
            if (object.margin != null)
                message.margin = String(object.margin);
            if (object.bgColor != null)
                message.bgColor = String(object.bgColor);
            if (object.textColor != null)
                message.textColor = String(object.textColor);
            if (object.border != null)
                message.border = String(object.border);
            if (object.rounded != null)
                message.rounded = String(object.rounded);
            if (object.shadow != null)
                message.shadow = String(object.shadow);
            if (object.gap != null)
                message.gap = String(object.gap);
            if (object.scrollable != null)
                message.scrollable = Boolean(object.scrollable);
            if (object.customClass != null)
                message.customClass = String(object.customClass);
            return message;
        };

        /**
         * Creates a plain object from a Style message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Style
         * @static
         * @param {debate.Style} message Style
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Style.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.defaults) {
                object.layout = options.enums === String ? "ABSOLUTE" : 0;
                object.position = null;
                object.size = null;
                object.padding = "";
                object.margin = "";
                object.bgColor = "";
                object.textColor = "";
                object.border = "";
                object.rounded = "";
                object.shadow = "";
                object.gap = "";
                object.scrollable = false;
                object.customClass = "";
            }
            if (message.layout != null && message.hasOwnProperty("layout"))
                object.layout = options.enums === String ? $root.debate.LayoutType[message.layout] === undefined ? message.layout : $root.debate.LayoutType[message.layout] : message.layout;
            if (message.position != null && message.hasOwnProperty("position"))
                object.position = $root.debate.Position.toObject(message.position, options);
            if (message.size != null && message.hasOwnProperty("size"))
                object.size = $root.debate.Size.toObject(message.size, options);
            if (message.padding != null && message.hasOwnProperty("padding"))
                object.padding = message.padding;
            if (message.margin != null && message.hasOwnProperty("margin"))
                object.margin = message.margin;
            if (message.bgColor != null && message.hasOwnProperty("bgColor"))
                object.bgColor = message.bgColor;
            if (message.textColor != null && message.hasOwnProperty("textColor"))
                object.textColor = message.textColor;
            if (message.border != null && message.hasOwnProperty("border"))
                object.border = message.border;
            if (message.rounded != null && message.hasOwnProperty("rounded"))
                object.rounded = message.rounded;
            if (message.shadow != null && message.hasOwnProperty("shadow"))
                object.shadow = message.shadow;
            if (message.gap != null && message.hasOwnProperty("gap"))
                object.gap = message.gap;
            if (message.scrollable != null && message.hasOwnProperty("scrollable"))
                object.scrollable = message.scrollable;
            if (message.customClass != null && message.hasOwnProperty("customClass"))
                object.customClass = message.customClass;
            return object;
        };

        /**
         * Converts this Style to JSON.
         * @function toJSON
         * @memberof debate.Style
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Style.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Style
         * @function getTypeUrl
         * @memberof debate.Style
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Style.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Style";
        };

        return Style;
    })();

    /**
     * LayoutType enum.
     * @name debate.LayoutType
     * @enum {number}
     * @property {number} ABSOLUTE=0 ABSOLUTE value
     * @property {number} FLEX_ROW=1 FLEX_ROW value
     * @property {number} FLEX_COL=2 FLEX_COL value
     * @property {number} GRID=3 GRID value
     */
    debate.LayoutType = (function() {
        const valuesById = {}, values = Object.create(valuesById);
        values[valuesById[0] = "ABSOLUTE"] = 0;
        values[valuesById[1] = "FLEX_ROW"] = 1;
        values[valuesById[2] = "FLEX_COL"] = 2;
        values[valuesById[3] = "GRID"] = 3;
        return values;
    })();

    debate.Position = (function() {

        /**
         * Properties of a Position.
         * @memberof debate
         * @interface IPosition
         * @property {number|null} [x] Position x
         * @property {number|null} [y] Position y
         * @property {string|null} [unit] Position unit
         */

        /**
         * Constructs a new Position.
         * @memberof debate
         * @classdesc Represents a Position.
         * @implements IPosition
         * @constructor
         * @param {debate.IPosition=} [properties] Properties to set
         */
        function Position(properties) {
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Position x.
         * @member {number} x
         * @memberof debate.Position
         * @instance
         */
        Position.prototype.x = 0;

        /**
         * Position y.
         * @member {number} y
         * @memberof debate.Position
         * @instance
         */
        Position.prototype.y = 0;

        /**
         * Position unit.
         * @member {string} unit
         * @memberof debate.Position
         * @instance
         */
        Position.prototype.unit = "";

        /**
         * Creates a new Position instance using the specified properties.
         * @function create
         * @memberof debate.Position
         * @static
         * @param {debate.IPosition=} [properties] Properties to set
         * @returns {debate.Position} Position instance
         */
        Position.create = function create(properties) {
            return new Position(properties);
        };

        /**
         * Encodes the specified Position message. Does not implicitly {@link debate.Position.verify|verify} messages.
         * @function encode
         * @memberof debate.Position
         * @static
         * @param {debate.IPosition} message Position message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Position.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.x != null && Object.hasOwnProperty.call(message, "x"))
                writer.uint32(/* id 1, wireType 5 =*/13).float(message.x);
            if (message.y != null && Object.hasOwnProperty.call(message, "y"))
                writer.uint32(/* id 2, wireType 5 =*/21).float(message.y);
            if (message.unit != null && Object.hasOwnProperty.call(message, "unit"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.unit);
            return writer;
        };

        /**
         * Encodes the specified Position message, length delimited. Does not implicitly {@link debate.Position.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Position
         * @static
         * @param {debate.IPosition} message Position message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Position.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Position message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Position
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Position} Position
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Position.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Position();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.x = reader.float();
                        break;
                    }
                case 2: {
                        message.y = reader.float();
                        break;
                    }
                case 3: {
                        message.unit = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Position message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Position
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Position} Position
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Position.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Position message.
         * @function verify
         * @memberof debate.Position
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Position.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.x != null && message.hasOwnProperty("x"))
                if (typeof message.x !== "number")
                    return "x: number expected";
            if (message.y != null && message.hasOwnProperty("y"))
                if (typeof message.y !== "number")
                    return "y: number expected";
            if (message.unit != null && message.hasOwnProperty("unit"))
                if (!$util.isString(message.unit))
                    return "unit: string expected";
            return null;
        };

        /**
         * Creates a Position message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Position
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Position} Position
         */
        Position.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Position)
                return object;
            let message = new $root.debate.Position();
            if (object.x != null)
                message.x = Number(object.x);
            if (object.y != null)
                message.y = Number(object.y);
            if (object.unit != null)
                message.unit = String(object.unit);
            return message;
        };

        /**
         * Creates a plain object from a Position message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Position
         * @static
         * @param {debate.Position} message Position
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Position.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.defaults) {
                object.x = 0;
                object.y = 0;
                object.unit = "";
            }
            if (message.x != null && message.hasOwnProperty("x"))
                object.x = options.json && !isFinite(message.x) ? String(message.x) : message.x;
            if (message.y != null && message.hasOwnProperty("y"))
                object.y = options.json && !isFinite(message.y) ? String(message.y) : message.y;
            if (message.unit != null && message.hasOwnProperty("unit"))
                object.unit = message.unit;
            return object;
        };

        /**
         * Converts this Position to JSON.
         * @function toJSON
         * @memberof debate.Position
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Position.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Position
         * @function getTypeUrl
         * @memberof debate.Position
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Position.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Position";
        };

        return Position;
    })();

    debate.Size = (function() {

        /**
         * Properties of a Size.
         * @memberof debate
         * @interface ISize
         * @property {string|null} [width] Size width
         * @property {string|null} [height] Size height
         */

        /**
         * Constructs a new Size.
         * @memberof debate
         * @classdesc Represents a Size.
         * @implements ISize
         * @constructor
         * @param {debate.ISize=} [properties] Properties to set
         */
        function Size(properties) {
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Size width.
         * @member {string} width
         * @memberof debate.Size
         * @instance
         */
        Size.prototype.width = "";

        /**
         * Size height.
         * @member {string} height
         * @memberof debate.Size
         * @instance
         */
        Size.prototype.height = "";

        /**
         * Creates a new Size instance using the specified properties.
         * @function create
         * @memberof debate.Size
         * @static
         * @param {debate.ISize=} [properties] Properties to set
         * @returns {debate.Size} Size instance
         */
        Size.create = function create(properties) {
            return new Size(properties);
        };

        /**
         * Encodes the specified Size message. Does not implicitly {@link debate.Size.verify|verify} messages.
         * @function encode
         * @memberof debate.Size
         * @static
         * @param {debate.ISize} message Size message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Size.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.width != null && Object.hasOwnProperty.call(message, "width"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.width);
            if (message.height != null && Object.hasOwnProperty.call(message, "height"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.height);
            return writer;
        };

        /**
         * Encodes the specified Size message, length delimited. Does not implicitly {@link debate.Size.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Size
         * @static
         * @param {debate.ISize} message Size message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Size.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Size message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Size
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Size} Size
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Size.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Size();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.width = reader.string();
                        break;
                    }
                case 2: {
                        message.height = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Size message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Size
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Size} Size
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Size.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Size message.
         * @function verify
         * @memberof debate.Size
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Size.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.width != null && message.hasOwnProperty("width"))
                if (!$util.isString(message.width))
                    return "width: string expected";
            if (message.height != null && message.hasOwnProperty("height"))
                if (!$util.isString(message.height))
                    return "height: string expected";
            return null;
        };

        /**
         * Creates a Size message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Size
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Size} Size
         */
        Size.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Size)
                return object;
            let message = new $root.debate.Size();
            if (object.width != null)
                message.width = String(object.width);
            if (object.height != null)
                message.height = String(object.height);
            return message;
        };

        /**
         * Creates a plain object from a Size message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Size
         * @static
         * @param {debate.Size} message Size
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Size.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.defaults) {
                object.width = "";
                object.height = "";
            }
            if (message.width != null && message.hasOwnProperty("width"))
                object.width = message.width;
            if (message.height != null && message.hasOwnProperty("height"))
                object.height = message.height;
            return object;
        };

        /**
         * Converts this Size to JSON.
         * @function toJSON
         * @memberof debate.Size
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Size.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Size
         * @function getTypeUrl
         * @memberof debate.Size
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Size.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Size";
        };

        return Size;
    })();

    debate.Theme = (function() {

        /**
         * Properties of a Theme.
         * @memberof debate
         * @interface ITheme
         * @property {string|null} [primaryColor] Theme primaryColor
         * @property {string|null} [secondaryColor] Theme secondaryColor
         * @property {string|null} [fontFamily] Theme fontFamily
         */

        /**
         * Constructs a new Theme.
         * @memberof debate
         * @classdesc Represents a Theme.
         * @implements ITheme
         * @constructor
         * @param {debate.ITheme=} [properties] Properties to set
         */
        function Theme(properties) {
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Theme primaryColor.
         * @member {string} primaryColor
         * @memberof debate.Theme
         * @instance
         */
        Theme.prototype.primaryColor = "";

        /**
         * Theme secondaryColor.
         * @member {string} secondaryColor
         * @memberof debate.Theme
         * @instance
         */
        Theme.prototype.secondaryColor = "";

        /**
         * Theme fontFamily.
         * @member {string} fontFamily
         * @memberof debate.Theme
         * @instance
         */
        Theme.prototype.fontFamily = "";

        /**
         * Creates a new Theme instance using the specified properties.
         * @function create
         * @memberof debate.Theme
         * @static
         * @param {debate.ITheme=} [properties] Properties to set
         * @returns {debate.Theme} Theme instance
         */
        Theme.create = function create(properties) {
            return new Theme(properties);
        };

        /**
         * Encodes the specified Theme message. Does not implicitly {@link debate.Theme.verify|verify} messages.
         * @function encode
         * @memberof debate.Theme
         * @static
         * @param {debate.ITheme} message Theme message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Theme.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.primaryColor != null && Object.hasOwnProperty.call(message, "primaryColor"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.primaryColor);
            if (message.secondaryColor != null && Object.hasOwnProperty.call(message, "secondaryColor"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.secondaryColor);
            if (message.fontFamily != null && Object.hasOwnProperty.call(message, "fontFamily"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.fontFamily);
            return writer;
        };

        /**
         * Encodes the specified Theme message, length delimited. Does not implicitly {@link debate.Theme.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.Theme
         * @static
         * @param {debate.ITheme} message Theme message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        Theme.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a Theme message from the specified reader or buffer.
         * @function decode
         * @memberof debate.Theme
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.Theme} Theme
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Theme.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.Theme();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.primaryColor = reader.string();
                        break;
                    }
                case 2: {
                        message.secondaryColor = reader.string();
                        break;
                    }
                case 3: {
                        message.fontFamily = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a Theme message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.Theme
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.Theme} Theme
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        Theme.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a Theme message.
         * @function verify
         * @memberof debate.Theme
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        Theme.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.primaryColor != null && message.hasOwnProperty("primaryColor"))
                if (!$util.isString(message.primaryColor))
                    return "primaryColor: string expected";
            if (message.secondaryColor != null && message.hasOwnProperty("secondaryColor"))
                if (!$util.isString(message.secondaryColor))
                    return "secondaryColor: string expected";
            if (message.fontFamily != null && message.hasOwnProperty("fontFamily"))
                if (!$util.isString(message.fontFamily))
                    return "fontFamily: string expected";
            return null;
        };

        /**
         * Creates a Theme message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.Theme
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.Theme} Theme
         */
        Theme.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.Theme)
                return object;
            let message = new $root.debate.Theme();
            if (object.primaryColor != null)
                message.primaryColor = String(object.primaryColor);
            if (object.secondaryColor != null)
                message.secondaryColor = String(object.secondaryColor);
            if (object.fontFamily != null)
                message.fontFamily = String(object.fontFamily);
            return message;
        };

        /**
         * Creates a plain object from a Theme message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.Theme
         * @static
         * @param {debate.Theme} message Theme
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        Theme.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.defaults) {
                object.primaryColor = "";
                object.secondaryColor = "";
                object.fontFamily = "";
            }
            if (message.primaryColor != null && message.hasOwnProperty("primaryColor"))
                object.primaryColor = message.primaryColor;
            if (message.secondaryColor != null && message.hasOwnProperty("secondaryColor"))
                object.secondaryColor = message.secondaryColor;
            if (message.fontFamily != null && message.hasOwnProperty("fontFamily"))
                object.fontFamily = message.fontFamily;
            return object;
        };

        /**
         * Converts this Theme to JSON.
         * @function toJSON
         * @memberof debate.Theme
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        Theme.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for Theme
         * @function getTypeUrl
         * @memberof debate.Theme
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        Theme.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.Theme";
        };

        return Theme;
    })();

    return debate;
})();

export { $root as default };
