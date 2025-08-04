/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
"use strict";

var $protobuf = require("protobufjs/minimal");

// Common aliases
var $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
var $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

$root.debate = (function() {

    /**
     * Namespace debate.
     * @exports debate
     * @namespace
     */
    var debate = {};

    debate.UIEvent = (function() {

        /**
         * Properties of a UIEvent.
         * @memberof debate
         * @interface IUIEvent
         * @property {string|null} [eventId] UIEvent eventId
         * @property {string|null} [componentId] UIEvent componentId
         * @property {debate.EventType|null} [type] UIEvent type
         * @property {number|Long|null} [timestamp] UIEvent timestamp
         * @property {Object.<string,string>|null} [data] UIEvent data
         */

        /**
         * Constructs a new UIEvent.
         * @memberof debate
         * @classdesc Represents a UIEvent.
         * @implements IUIEvent
         * @constructor
         * @param {debate.IUIEvent=} [properties] Properties to set
         */
        function UIEvent(properties) {
            this.data = {};
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * UIEvent eventId.
         * @member {string} eventId
         * @memberof debate.UIEvent
         * @instance
         */
        UIEvent.prototype.eventId = "";

        /**
         * UIEvent componentId.
         * @member {string} componentId
         * @memberof debate.UIEvent
         * @instance
         */
        UIEvent.prototype.componentId = "";

        /**
         * UIEvent type.
         * @member {debate.EventType} type
         * @memberof debate.UIEvent
         * @instance
         */
        UIEvent.prototype.type = 0;

        /**
         * UIEvent timestamp.
         * @member {number|Long} timestamp
         * @memberof debate.UIEvent
         * @instance
         */
        UIEvent.prototype.timestamp = $util.Long ? $util.Long.fromBits(0,0,false) : 0;

        /**
         * UIEvent data.
         * @member {Object.<string,string>} data
         * @memberof debate.UIEvent
         * @instance
         */
        UIEvent.prototype.data = $util.emptyObject;

        /**
         * Creates a new UIEvent instance using the specified properties.
         * @function create
         * @memberof debate.UIEvent
         * @static
         * @param {debate.IUIEvent=} [properties] Properties to set
         * @returns {debate.UIEvent} UIEvent instance
         */
        UIEvent.create = function create(properties) {
            return new UIEvent(properties);
        };

        /**
         * Encodes the specified UIEvent message. Does not implicitly {@link debate.UIEvent.verify|verify} messages.
         * @function encode
         * @memberof debate.UIEvent
         * @static
         * @param {debate.IUIEvent} message UIEvent message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        UIEvent.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.eventId != null && Object.hasOwnProperty.call(message, "eventId"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.eventId);
            if (message.componentId != null && Object.hasOwnProperty.call(message, "componentId"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.componentId);
            if (message.type != null && Object.hasOwnProperty.call(message, "type"))
                writer.uint32(/* id 3, wireType 0 =*/24).int32(message.type);
            if (message.timestamp != null && Object.hasOwnProperty.call(message, "timestamp"))
                writer.uint32(/* id 4, wireType 0 =*/32).int64(message.timestamp);
            if (message.data != null && Object.hasOwnProperty.call(message, "data"))
                for (var keys = Object.keys(message.data), i = 0; i < keys.length; ++i)
                    writer.uint32(/* id 5, wireType 2 =*/42).fork().uint32(/* id 1, wireType 2 =*/10).string(keys[i]).uint32(/* id 2, wireType 2 =*/18).string(message.data[keys[i]]).ldelim();
            return writer;
        };

        /**
         * Encodes the specified UIEvent message, length delimited. Does not implicitly {@link debate.UIEvent.verify|verify} messages.
         * @function encodeDelimited
         * @memberof debate.UIEvent
         * @static
         * @param {debate.IUIEvent} message UIEvent message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        UIEvent.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a UIEvent message from the specified reader or buffer.
         * @function decode
         * @memberof debate.UIEvent
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {debate.UIEvent} UIEvent
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        UIEvent.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.debate.UIEvent(), key, value;
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.eventId = reader.string();
                        break;
                    }
                case 2: {
                        message.componentId = reader.string();
                        break;
                    }
                case 3: {
                        message.type = reader.int32();
                        break;
                    }
                case 4: {
                        message.timestamp = reader.int64();
                        break;
                    }
                case 5: {
                        if (message.data === $util.emptyObject)
                            message.data = {};
                        var end2 = reader.uint32() + reader.pos;
                        key = "";
                        value = "";
                        while (reader.pos < end2) {
                            var tag2 = reader.uint32();
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
                        message.data[key] = value;
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
         * Decodes a UIEvent message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof debate.UIEvent
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {debate.UIEvent} UIEvent
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        UIEvent.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a UIEvent message.
         * @function verify
         * @memberof debate.UIEvent
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        UIEvent.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.eventId != null && message.hasOwnProperty("eventId"))
                if (!$util.isString(message.eventId))
                    return "eventId: string expected";
            if (message.componentId != null && message.hasOwnProperty("componentId"))
                if (!$util.isString(message.componentId))
                    return "componentId: string expected";
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
                    break;
                }
            if (message.timestamp != null && message.hasOwnProperty("timestamp"))
                if (!$util.isInteger(message.timestamp) && !(message.timestamp && $util.isInteger(message.timestamp.low) && $util.isInteger(message.timestamp.high)))
                    return "timestamp: integer|Long expected";
            if (message.data != null && message.hasOwnProperty("data")) {
                if (!$util.isObject(message.data))
                    return "data: object expected";
                var key = Object.keys(message.data);
                for (var i = 0; i < key.length; ++i)
                    if (!$util.isString(message.data[key[i]]))
                        return "data: string{k:string} expected";
            }
            return null;
        };

        /**
         * Creates a UIEvent message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof debate.UIEvent
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {debate.UIEvent} UIEvent
         */
        UIEvent.fromObject = function fromObject(object) {
            if (object instanceof $root.debate.UIEvent)
                return object;
            var message = new $root.debate.UIEvent();
            if (object.eventId != null)
                message.eventId = String(object.eventId);
            if (object.componentId != null)
                message.componentId = String(object.componentId);
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
            case "CLICK":
            case 1:
                message.type = 1;
                break;
            case "INPUT_CHANGE":
            case 2:
                message.type = 2;
                break;
            case "FORM_SUBMIT":
            case 3:
                message.type = 3;
                break;
            case "NODE_ADDED":
            case 4:
                message.type = 4;
                break;
            case "NODE_REMOVED":
            case 5:
                message.type = 5;
                break;
            case "EDGE_ADDED":
            case 6:
                message.type = 6;
                break;
            case "EDGE_REMOVED":
            case 7:
                message.type = 7;
                break;
            }
            if (object.timestamp != null)
                if ($util.Long)
                    (message.timestamp = $util.Long.fromValue(object.timestamp)).unsigned = false;
                else if (typeof object.timestamp === "string")
                    message.timestamp = parseInt(object.timestamp, 10);
                else if (typeof object.timestamp === "number")
                    message.timestamp = object.timestamp;
                else if (typeof object.timestamp === "object")
                    message.timestamp = new $util.LongBits(object.timestamp.low >>> 0, object.timestamp.high >>> 0).toNumber();
            if (object.data) {
                if (typeof object.data !== "object")
                    throw TypeError(".debate.UIEvent.data: object expected");
                message.data = {};
                for (var keys = Object.keys(object.data), i = 0; i < keys.length; ++i)
                    message.data[keys[i]] = String(object.data[keys[i]]);
            }
            return message;
        };

        /**
         * Creates a plain object from a UIEvent message. Also converts values to other types if specified.
         * @function toObject
         * @memberof debate.UIEvent
         * @static
         * @param {debate.UIEvent} message UIEvent
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        UIEvent.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.objects || options.defaults)
                object.data = {};
            if (options.defaults) {
                object.eventId = "";
                object.componentId = "";
                object.type = options.enums === String ? "UNKNOWN" : 0;
                if ($util.Long) {
                    var long = new $util.Long(0, 0, false);
                    object.timestamp = options.longs === String ? long.toString() : options.longs === Number ? long.toNumber() : long;
                } else
                    object.timestamp = options.longs === String ? "0" : 0;
            }
            if (message.eventId != null && message.hasOwnProperty("eventId"))
                object.eventId = message.eventId;
            if (message.componentId != null && message.hasOwnProperty("componentId"))
                object.componentId = message.componentId;
            if (message.type != null && message.hasOwnProperty("type"))
                object.type = options.enums === String ? $root.debate.EventType[message.type] === undefined ? message.type : $root.debate.EventType[message.type] : message.type;
            if (message.timestamp != null && message.hasOwnProperty("timestamp"))
                if (typeof message.timestamp === "number")
                    object.timestamp = options.longs === String ? String(message.timestamp) : message.timestamp;
                else
                    object.timestamp = options.longs === String ? $util.Long.prototype.toString.call(message.timestamp) : options.longs === Number ? new $util.LongBits(message.timestamp.low >>> 0, message.timestamp.high >>> 0).toNumber() : message.timestamp;
            var keys2;
            if (message.data && (keys2 = Object.keys(message.data)).length) {
                object.data = {};
                for (var j = 0; j < keys2.length; ++j)
                    object.data[keys2[j]] = message.data[keys2[j]];
            }
            return object;
        };

        /**
         * Converts this UIEvent to JSON.
         * @function toJSON
         * @memberof debate.UIEvent
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        UIEvent.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for UIEvent
         * @function getTypeUrl
         * @memberof debate.UIEvent
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        UIEvent.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/debate.UIEvent";
        };

        return UIEvent;
    })();

    /**
     * EventType enum.
     * @name debate.EventType
     * @enum {number}
     * @property {number} UNKNOWN=0 UNKNOWN value
     * @property {number} CLICK=1 CLICK value
     * @property {number} INPUT_CHANGE=2 INPUT_CHANGE value
     * @property {number} FORM_SUBMIT=3 FORM_SUBMIT value
     * @property {number} NODE_ADDED=4 NODE_ADDED value
     * @property {number} NODE_REMOVED=5 NODE_REMOVED value
     * @property {number} EDGE_ADDED=6 EDGE_ADDED value
     * @property {number} EDGE_REMOVED=7 EDGE_REMOVED value
     */
    debate.EventType = (function() {
        var valuesById = {}, values = Object.create(valuesById);
        values[valuesById[0] = "UNKNOWN"] = 0;
        values[valuesById[1] = "CLICK"] = 1;
        values[valuesById[2] = "INPUT_CHANGE"] = 2;
        values[valuesById[3] = "FORM_SUBMIT"] = 3;
        values[valuesById[4] = "NODE_ADDED"] = 4;
        values[valuesById[5] = "NODE_REMOVED"] = 5;
        values[valuesById[6] = "EDGE_ADDED"] = 6;
        values[valuesById[7] = "EDGE_REMOVED"] = 7;
        return values;
    })();

    return debate;
})();

module.exports = $root;
