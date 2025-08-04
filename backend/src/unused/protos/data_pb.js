/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
import * as $protobuf from "protobufjs/minimal";

// Common aliases
const $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
const $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

export const test = $root.test = (() => {

    /**
     * Namespace test.
     * @exports test
     * @namespace
     */
    const test = {};

    test.MyData = (function() {

        /**
         * Properties of a MyData.
         * @memberof test
         * @interface IMyData
         * @property {string|null} [username] MyData username
         * @property {number|null} [score] MyData score
         */

        /**
         * Constructs a new MyData.
         * @memberof test
         * @classdesc Represents a MyData.
         * @implements IMyData
         * @constructor
         * @param {test.IMyData=} [properties] Properties to set
         */
        function MyData(properties) {
            if (properties)
                for (let keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * MyData username.
         * @member {string} username
         * @memberof test.MyData
         * @instance
         */
        MyData.prototype.username = "";

        /**
         * MyData score.
         * @member {number} score
         * @memberof test.MyData
         * @instance
         */
        MyData.prototype.score = 0;

        /**
         * Creates a new MyData instance using the specified properties.
         * @function create
         * @memberof test.MyData
         * @static
         * @param {test.IMyData=} [properties] Properties to set
         * @returns {test.MyData} MyData instance
         */
        MyData.create = function create(properties) {
            return new MyData(properties);
        };

        /**
         * Encodes the specified MyData message. Does not implicitly {@link test.MyData.verify|verify} messages.
         * @function encode
         * @memberof test.MyData
         * @static
         * @param {test.IMyData} message MyData message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        MyData.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.username != null && Object.hasOwnProperty.call(message, "username"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.username);
            if (message.score != null && Object.hasOwnProperty.call(message, "score"))
                writer.uint32(/* id 2, wireType 0 =*/16).int32(message.score);
            return writer;
        };

        /**
         * Encodes the specified MyData message, length delimited. Does not implicitly {@link test.MyData.verify|verify} messages.
         * @function encodeDelimited
         * @memberof test.MyData
         * @static
         * @param {test.IMyData} message MyData message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        MyData.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a MyData message from the specified reader or buffer.
         * @function decode
         * @memberof test.MyData
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {test.MyData} MyData
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        MyData.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            let end = length === undefined ? reader.len : reader.pos + length, message = new $root.test.MyData();
            while (reader.pos < end) {
                let tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.username = reader.string();
                        break;
                    }
                case 2: {
                        message.score = reader.int32();
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
         * Decodes a MyData message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof test.MyData
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {test.MyData} MyData
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        MyData.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a MyData message.
         * @function verify
         * @memberof test.MyData
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        MyData.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.username != null && message.hasOwnProperty("username"))
                if (!$util.isString(message.username))
                    return "username: string expected";
            if (message.score != null && message.hasOwnProperty("score"))
                if (!$util.isInteger(message.score))
                    return "score: integer expected";
            return null;
        };

        /**
         * Creates a MyData message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof test.MyData
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {test.MyData} MyData
         */
        MyData.fromObject = function fromObject(object) {
            if (object instanceof $root.test.MyData)
                return object;
            let message = new $root.test.MyData();
            if (object.username != null)
                message.username = String(object.username);
            if (object.score != null)
                message.score = object.score | 0;
            return message;
        };

        /**
         * Creates a plain object from a MyData message. Also converts values to other types if specified.
         * @function toObject
         * @memberof test.MyData
         * @static
         * @param {test.MyData} message MyData
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        MyData.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            let object = {};
            if (options.defaults) {
                object.username = "";
                object.score = 0;
            }
            if (message.username != null && message.hasOwnProperty("username"))
                object.username = message.username;
            if (message.score != null && message.hasOwnProperty("score"))
                object.score = message.score;
            return object;
        };

        /**
         * Converts this MyData to JSON.
         * @function toJSON
         * @memberof test.MyData
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        MyData.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for MyData
         * @function getTypeUrl
         * @memberof test.MyData
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        MyData.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/test.MyData";
        };

        return MyData;
    })();

    test.DataService = (function() {

        /**
         * Constructs a new DataService service.
         * @memberof test
         * @classdesc Represents a DataService
         * @extends $protobuf.rpc.Service
         * @constructor
         * @param {$protobuf.RPCImpl} rpcImpl RPC implementation
         * @param {boolean} [requestDelimited=false] Whether requests are length-delimited
         * @param {boolean} [responseDelimited=false] Whether responses are length-delimited
         */
        function DataService(rpcImpl, requestDelimited, responseDelimited) {
            $protobuf.rpc.Service.call(this, rpcImpl, requestDelimited, responseDelimited);
        }

        (DataService.prototype = Object.create($protobuf.rpc.Service.prototype)).constructor = DataService;

        /**
         * Creates new DataService service using the specified rpc implementation.
         * @function create
         * @memberof test.DataService
         * @static
         * @param {$protobuf.RPCImpl} rpcImpl RPC implementation
         * @param {boolean} [requestDelimited=false] Whether requests are length-delimited
         * @param {boolean} [responseDelimited=false] Whether responses are length-delimited
         * @returns {DataService} RPC service. Useful where requests and/or responses are streamed.
         */
        DataService.create = function create(rpcImpl, requestDelimited, responseDelimited) {
            return new this(rpcImpl, requestDelimited, responseDelimited);
        };

        /**
         * Callback as used by {@link test.DataService#sendData}.
         * @memberof test.DataService
         * @typedef SendDataCallback
         * @type {function}
         * @param {Error|null} error Error, if any
         * @param {test.MyData} [response] MyData
         */

        /**
         * Calls SendData.
         * @function sendData
         * @memberof test.DataService
         * @instance
         * @param {test.IMyData} request MyData message or plain object
         * @param {test.DataService.SendDataCallback} callback Node-style callback called with the error, if any, and MyData
         * @returns {undefined}
         * @variation 1
         */
        Object.defineProperty(DataService.prototype.sendData = function sendData(request, callback) {
            return this.rpcCall(sendData, $root.test.MyData, $root.test.MyData, request, callback);
        }, "name", { value: "SendData" });

        /**
         * Calls SendData.
         * @function sendData
         * @memberof test.DataService
         * @instance
         * @param {test.IMyData} request MyData message or plain object
         * @returns {Promise<test.MyData>} Promise
         * @variation 2
         */

        return DataService;
    })();

    return test;
})();

export { $root as default };
