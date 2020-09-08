//
// Created by Matthew.Sirman on 26/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_PROTOCOLINTERNAL_H
#define CONTRACTS_SITE_CLIENT_PROTOCOLINTERNAL_H

#include <functional>

namespace networking {

    // Forward declare the protocol class
    class Protocol;

    template<typename _Layer>
    struct LayerReference;

    namespace internal {

        // role_sender_t
        // Represents a tag structure for role tagging as sender.
        struct role_sender_t {
            // Internal construction
            enum _Construct {
                _Token
            };

            // Constructor
            explicit constexpr role_sender_t(_Construct) {}
        };

        // role_receiver_t
        // Represents a tag structure for role tagging as receiver
        struct role_receiver_t {
            // Internal construction
            enum _Construct {
                _Token
            };

            // Constructor
            explicit constexpr role_receiver_t(_Construct) {}
        };

        // ProtocolLayer
        // Base type for a layer in the protocol
        struct ProtocolLayer {
            // Default constructor
            ProtocolLayer() = default;

            // Default constructor for tagging as Sender
            ProtocolLayer(role_sender_t) {};

            // Default constructor for tagging as Receiver
            ProtocolLayer(role_receiver_t) {};

            // Pure virtual activation function. This is called for each layer when the protocol is executed.
            virtual void activate() = 0;

            void markProtocolTermination();

            bool protocolTerminated() const;

            void reset();

        private:
            bool terminateProtocol = false;
        };

        // ParameterValue
        // Represents a dynamically polymorphic parameter of any type by masking the interfacing through templates.
        // Essentially a templated wrapper for a void pointer
        struct ParameterValue {
        public:
            // Templated constructor taking a true value for this ParameterValue to store
            template<typename _Ty>
            ParameterValue(const _Ty &value);

            // Templated getter for inferring the internal value as a certain type - note this is NOT type safe
            // and so should not be used outside of internal mechanisms (where type checking is performed at a higher
            // level)
            template<typename _Ty>
            const _Ty &get() const;

            // Templated setter for setting the internal value to a certain type
            template<typename _Ty>
            void set(const _Ty &value);

        private:
            // Pointer to the actual data. This is a void pointer as we do not know the type at compile time
            // (a void pointer can take any type)
            const void *__data;
        };

        // ParameterGroup
        // Represents a group of dynamically polymorphic parameters of any type by masking the interfacing through
        // templates.
        // Essentially a wrapper for an arbitrary set of void pointers
        struct ParameterGroup {
        public:
            // Static constructor which specifies the parameter set this should take,
            // allowing the internal vector to be constructed to the correct size
            template<typename ..._Group>
            static ParameterGroup create();

            // Getter for a specific element at the given index
            template<typename _Ty>
            const _Ty &get(size_t index) const;

            // Setter for a specific element at the given index
            template<typename _Ty>
            void set(size_t index, const _Ty &value);

            // Return if the group is fully filled or not
            bool ready() const;

            // Clears the parameter set
            // Note this does not delete the values at the parameters, it simply removes the internal reference
            // to them.
            void clear();

        private:
            // Internal set of parameters stored as void pointers for the dynamic polymorphism
            std::vector<const void *> __parameters;
        };

        // Connector
        // Represents a connectable slot between two protocol layers.
        template<size_t _index, typename _Layer, typename _Ty>
        struct Connector {
            // Friend the Protocol class so it can access the internal feed and read functions which are not type safe
            // in isolation
            friend class networking::Protocol;

            // Alias the _Layer type for external use
            using LayerType = _Layer;
            // Alias the _Ty type for external use
            using ValueType = _Ty;

            // Transformation alias to transform the slot on a pre-existing layer to be used on
            // a new layer without instantiating a "new" connector from scratch.
            // Note that by default, the index of the aliased slot will be set equivalent to the
            // original slot, but this can be changed.
            template<typename _OutLayer, size_t _outIndex = _index>
            using AsSlotFor = Connector<_outIndex, _OutLayer, _Ty>;

            // Static getter for the parameter index - we cannot alias a value, so instead we declare a constexpr
            static constexpr size_t paramIndex();

            // Getter for the internal value stored in the connector
            _Ty &get();

        private:
            // Feeds a value into this connector
            void feed(const _Ty &value);

            // Reads the value from this connector
            const _Ty &read() const;

            // The actual data element stored in this connector - each connector
            // must own a copy of the value it references
            ValueType data;
        };

        // input_layer_t
        // Represents a tag structure for input layer tagging. This means it is not necessary to
        // have a reference to the input layer when using it; the protocol will be able to infer the layer
        // from just knowing the tag
        struct input_layer_t {
            // Internal construction
            enum _Construct {
                _Token
            };

            // Constructor
            explicit constexpr input_layer_t(_Construct) {}
        };

        // output_layer_t
        // Represents a tag structure for output layer tagging. This means it is not necessary to
        // have a reference to the output layer when using it; the protocol will be able to infer the layer
        // from just knowing the tag
        struct output_layer_t {
            // Internal construction
            enum _Construct {
                _Token
            };

            // Constructor
            explicit constexpr output_layer_t(_Construct) {}
        };

        // Base Declaration for switcher type
        template<bool, typename _Ty>
        struct InputOrLayerSwitchType {

        };

        // Case if the bool is true - this represents a type which is a layer type
        template<typename _Ty>
        struct InputOrLayerSwitchType<true, _Ty> {
            // Set the type to a const LayerReference of the layer type
            using Type = const networking::LayerReference<_Ty> &;
        };

        // Case if the bool is false - this represents an input layer type
        template<typename _Ty>
        struct InputOrLayerSwitchType<false, _Ty> {
            // Set the type to just a plain input layer type
            using Type = input_layer_t;
        };

        // InputOrLayerSwitch
        // The Type parameter is either filled with input_layer_t or const LayerReference<_Layer> & depending
        // if _Layer inherits from ProtocolLayer.
        template<typename _Layer>
        struct InputOrLayerSwitch {
            // Set the type to the type from the switcher, with the bool parameter filled by the std::is_base_of_v
            // template
            using Type = typename InputOrLayerSwitchType<std::is_base_of_v<ProtocolLayer, _Layer>, _Layer>::Type;
        };

        inline void ProtocolLayer::markProtocolTermination() {
            terminateProtocol = true;
        }

        inline bool ProtocolLayer::protocolTerminated() const {
            return terminateProtocol;
        }

        inline void ProtocolLayer::reset() {
            terminateProtocol = false;
        }

        template<typename _Ty>
        ParameterValue::ParameterValue(const _Ty &value) {
            // Set the internal void pointer to the address of whatever value is passed in
            __data = &value;
        }

        template<typename _Ty>
        const _Ty &ParameterValue::get() const {
            // Infer the data pointer as the templated type and dereference it - this may not throw an exception
            // if the type is invalid so type checking must be done!
            return *((_Ty *) __data);
        }

        template<typename _Ty>
        void ParameterValue::set(const _Ty &value) {
            // Set the internal void pointer to the address of whatever value is passed in
            __data = &value;
        }

        template<typename... _Group>
        ParameterGroup ParameterGroup::create() {
            // Create the group
            ParameterGroup group;
            // Resize the parameters list
            group.__parameters.resize(sizeof...(_Group));
            return group;
        }

        template<typename _Ty>
        const _Ty &ParameterGroup::get(size_t index) const {
            return *((_Ty *) __parameters[index]);
        }

        template<typename _Ty>
        void ParameterGroup::set(size_t index, const _Ty &value) {
            __parameters[index] = &value;
        }

        inline bool ParameterGroup::ready() const {
            // Return that we are ready if every pointer is not null
            return std::all_of(__parameters.begin(), __parameters.end(), [](const void *p) { return p; });
        }

        inline void ParameterGroup::clear() {
            // Set each pointer to a nullptr
            std::for_each(__parameters.begin(), __parameters.end(), [](const void *&p) { p = nullptr; });
        }

        template<size_t _index, typename _Layer, typename _Ty>
        constexpr size_t Connector<_index, _Layer, _Ty>::paramIndex() {
            // Return the index template parameter
            return _index;
        }

        template<size_t _index, typename _Layer, typename _Ty>
        void Connector<_index, _Layer, _Ty>::feed(const _Ty &value) {
            data = value;
        }

        template<size_t _index, typename _Layer, typename _Ty>
        const _Ty &Connector<_index, _Layer, _Ty>::read() const {
            return data;
        }

        template<size_t _index, typename _Layer, typename _Ty>
        _Ty &Connector<_index, _Layer, _Ty>::get() {
            // Return the internal data as its actual type
            return data;
        }

        // Expands the transform function arguments with their appropriate indices
        template<typename _Ret, typename ..._Args, size_t ..._indices>
        const _Ret &__transformerExpander(const std::function<const _Ret &(const typename _Args::ValueType &...)> &transform,
                                  ParameterGroup &group, std::integer_sequence<size_t, _indices...>) {
            return transform(group.get<typename _Args::ValueType>(_indices)...);
        }


    }

    // Tag literal for the sender role
    inline constexpr internal::role_sender_t Sender{internal::role_sender_t::_Construct::_Token};
    // Tag literal for the receiver role
    inline constexpr internal::role_receiver_t Receiver{internal::role_receiver_t::_Construct::_Token};

}

#endif //CONTRACTS_SITE_CLIENT_PROTOCOLINTERNAL_H
