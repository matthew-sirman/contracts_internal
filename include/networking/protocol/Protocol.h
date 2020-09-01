//
// Created by Matthew.Sirman on 25/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_PROTOCOL_H
#define CONTRACTS_SITE_CLIENT_PROTOCOL_H

#include <type_traits>
#include <vector>
#include <set>
#include <memory>
#include <functional>
#include <algorithm>

#include "layers/KeyExchange.h"
#include "layers/RSAMessageLayer.h"
#include "layers/PrimitiveExchange.h"

namespace networking {

    // LayerReference<_Layer>
    // Represents an indirect reference to a protocol layer for interface level manipulation without
    // the ability to access the internal mechanisms
    template<typename _Layer>
    struct LayerReference {
        // Friend the Protocol class so it can access the private references
        friend class Protocol;

    public:

    private:
        // Private constructor taking the reference to the concerning layer and the index of that layer
        // in the protocol
        LayerReference(_Layer &layerRef, size_t index);

        // Reference to the actual layer object
        _Layer &__ref;
        // Index parameter for this layer (i.e. 0 -> first layer etc.)
        size_t __index;
    };

    // Protocol
    // A layered protocol into which data can be fed, which processes data, and then returns some set of
    // outputs. Works through templated linking of internal mechanisms
    class Protocol {
    public:
        // Tag literal for the input layer
        inline static constexpr internal::input_layer_t Inputs{internal::input_layer_t::_Construct::_Token};
        // Tag literal for the output layer
        inline static constexpr internal::output_layer_t Outputs{internal::output_layer_t::_Construct::_Token};

        Protocol() = default;

        Protocol(const Protocol &protocol) = delete;

        Protocol(Protocol &&protocol) noexcept;

        ~Protocol() = default;

        Protocol &operator=(const Protocol &other) = delete;

        Protocol &operator=(Protocol &&other) noexcept;

        // Add a layer of the given type with no specific role
        template<typename _Layer>
        LayerReference<_Layer> addLayer();

        // Add a layer of the given type with the Sender role, namely in the network protocol this layer
        // should send data out
        template<typename _Layer>
        LayerReference<_Layer> addLayer(internal::role_sender_t);

        // Add a layer of the given type with the Receiver role, namely in the network protocol this layer
        // should pull data in
        template<typename _Layer>
        LayerReference<_Layer> addLayer(internal::role_receiver_t);

        // Link two intermediary layers together. The _From parameter will feed into the _To parameter
        // from layerFrom to layerTo under the identity function when the model is executed
        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  const LayerReference<typename _To::LayerType> &layerTo);

        // Link two intermediary layers together with a transformation function mapping the value type of the _From
        // parameter to the value type of the _To parameter on the two specified layers
        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  const LayerReference<typename _To::LayerType> &layerTo,
                  const std::function<const typename _To::ValueType &(const typename _From::ValueType &)> &transform);

        // Link the input layer to an intermediary layer. The _From parameter will feed into the _To parameter,
        // under the identity function, on layerTo when the feed function is called with the _From parameter.
        // This allows data inputting into the model.
        // Note: this link is a one-to-many relationship - namely a single input parameter can be fed into multiple
        // layers and slots (as long as the types match)
        template<typename _From, typename _To>
        void link(internal::input_layer_t,
                  const LayerReference<typename _To::LayerType> &layerTo);

        // Link the input layer to an intermediary layer. The _From parameter will feed into the _To parameter,
        // under the transform function, on layerTo when the feed function is called with the _From parameter.
        // This allows data inputting into the model.
        // Note: this link is a one-to-many relationship - namely a single input parameter can be fed into multiple
        // layers and slots (as long as the types match)
        template<typename _From, typename _To>
        void link(internal::input_layer_t,
                  const LayerReference<typename _To::LayerType> &layerTo,
                  const std::function<const typename _To::ValueType &(const typename _From::ValueType &)> &transform);

        // Link an intermediary layer to the output layer. The _From parameter on layerFrom will feed into the _To
        // parameter, under the identity function, of the output when the read function is called with the _To parameter.
        // This allows data extraction from the model after completion.
        // Note: this link is a one-to-one relationship - namely a single model parameter can feed into at most one
        // output parameter
        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  internal::output_layer_t);

        // Link an intermediary layer to the output layer. The _From parameter on layerFrom will feed into the _To
        // parameter, under the transform function, of the output when the read function is called with the _To parameter.
        // This allows data extraction from the model after completion.
        // Note: this link is a one-to-one relationship - namely a single model parameter can feed into at most one
        // output parameter
        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  internal::output_layer_t,
                  std::function<const typename _To::ValueType &(const typename _From::ValueType &)> &transform);

        // Link some set of parameters into a single input slot on a given layer. The inputs can come from intermediary
        // layers, or from the input. Once all of the required inputs are fed in, the transformer function is executed
        // and all of the parameters are fed onwards.
        // Note that the first packed parameter is of type InputOrLayerSwitch. This type will expand to
        // LayerReference<typename _From::LayerType> if the layer at that point is a non-input layer, otherwise will
        // expand to input_layer_t.
        template<typename _To, typename ..._From>
        void multiLink(typename internal::InputOrLayerSwitch<typename _From::LayerType>::Type ...layersFrom,
                       const LayerReference<typename _To::LayerType> &layerTo,
                       const std::function<const typename _To::ValueType &(
                               const typename _From::ValueType &...)> &transform);

        // Feed the given value into the parameter slot specified
        template<typename _Param>
        void feed(const typename _Param::ValueType &value);

        // Read the output value from the parameter slot specified
        template<typename _Param>
        typename _Param::ValueType read();

        // Execute the model. This will call all the linker and activation functions on each layer
        void execute();

        // Clear the current data in the protocol
        void clearData();

    private:
        // Alias a "link" element, namely a function and its corresponding layer. This allows for
        // storing a set of link functions sorted by the layer they are on
        using LinkElement = std::pair<size_t, std::function<void()>>;

        // LinkComparator
        // Structure for comparing two link elements by their layer index
        struct LinkComparator {
            // Comparator operator
            bool operator()(const LinkElement &lhs,
                            const LinkElement &rhs) const;
        };

        // Recursive function to add each required link slot to the protocol. This version of the function call
        // is enabled if the head _From slot is from an intermediary layer, not the input layer.
        template<typename _To, typename _From, typename ..._Rest>
        void addMultiLinks(size_t index, internal::ParameterGroup &parameterGroup,
                            typename std::enable_if<std::is_base_of_v<internal::ProtocolLayer, typename _From::LayerType>,
                                    const LayerReference<typename _From::LayerType> &>::type layerFrom,
                            typename internal::InputOrLayerSwitch<_Rest>::Type ...restFrom,
                            typename _To::LayerType &layerTo,
                            const std::function<const typename _To::ValueType &(internal::ParameterGroup &)> &transform);

        // Recursive function to add each required feed slot to the protocol. This version of the function call
        // is enabled if the head _From slot is from the input layer.
        template<typename _To, typename _From, typename ..._Rest>
        void addMultiLinks(size_t index, internal::ParameterGroup &parameterGroup,
                            typename std::enable_if<!std::is_base_of_v<internal::ProtocolLayer, typename _From::LayerType>,
                                    internal::input_layer_t>::type layerFrom,
                            typename internal::InputOrLayerSwitch<_Rest>::Type ...restFrom,
                            typename _To::LayerType &layerTo,
                            const std::function<const typename _To::ValueType &(internal::ParameterGroup &)> &transform);

        // Base case for the recursive function. This is enabled only if there are no more layers to add, an so has
        // an empty body
        template<typename _To, typename ..._Rest>
        typename std::enable_if<sizeof...(_Rest) == 0>::type
        addMultiLinks(size_t, internal::ParameterGroup &, typename _To::LayerType &,
                       const std::function<const typename _To::ValueType &(internal::ParameterGroup &)> &) {}

        // Vector of layers stored as pointers such that they can be of any type derived from ProtocolLayer.
        // The protocol is the sole owner of these layers (as they can not be accessed from outside code directly
        // only through indirect reference structures) and so they are stored as unique pointers
        std::vector<std::unique_ptr<internal::ProtocolLayer>> layers;

        // One-to-many mapping from input parameter slots (represented here as integers) to feed functions.
        // Once these links are set up, a feed of parameter i will feed the given value to all links in the
        // vector pertaining to i.
        std::unordered_map<size_t, std::vector<std::function<void(internal::ParameterValue)>>> feeds;
        // Set of link functions. This is stored in a set with an explicit comparator, so the elements
        // will always be ordered by their layer upon insertion - we want to execute the linkers in the order
        // of the layer indices
        std::set<LinkElement, LinkComparator> links;
        // One-to-one mapping from output slots to output functions
        std::unordered_map<size_t, std::function<internal::ParameterValue()>> outputs;

        // Grouped parameter sets for multi-links
        std::vector<internal::ParameterGroup> parameterGroups;

        // Counter helper variable for the current layer index of the protocol. Incremented each time a layer
        // is added
        size_t currentLayerIndex = 0;
    };

    // Alias for an internal protocol layer in the networking namespace. Allows for user defined layers to
    // inherit type Layer without needing to enter internal namespace.
    using Layer = internal::ProtocolLayer;

    template<typename _Layer>
    LayerReference<_Layer>::LayerReference(_Layer &layerRef, size_t index)
            : __ref(layerRef), __index(index) {

    }

    template<typename _Layer>
    LayerReference<_Layer> Protocol::addLayer() {
        // First create the new layer on the heap (so it can be of type _Layer) with no usage hint
        std::unique_ptr<_Layer> layer(new _Layer());
        // Construct a layer reference object - this takes the reference to the object itself and also
        // the current layer index (which is then incremented)
        LayerReference<_Layer> layerRef(*layer, currentLayerIndex++);

        // Add the pointer to the back of the layers vector by moving (so we are not duplicating memory
        // and have only a single reference to that piece of memory)
        layers.push_back(std::move(layer));

        // Return the reference object for interfacing use
        return layerRef;
    }

    template<typename _Layer>
    LayerReference<_Layer> Protocol::addLayer(internal::role_sender_t) {
        // First create the new layer on the heap (so it can be of type _Layer) with the sender tag. If the layer
        // has different behaviour depending on its role, it can distinguish its role through having a different
        // constructor for the sender and receiver tags
        std::unique_ptr<_Layer> layer(new _Layer(Sender));
        // Construct a layer reference object - this takes the reference to the object itself and also
        // the current layer index (which is then incremented)
        LayerReference<_Layer> layerRef(*layer, currentLayerIndex++);

        // Add the pointer to the back of the layers vector by moving (so we are not duplicating memory
        // and have only a single reference to that piece of memory)
        layers.push_back(std::move(layer));

        // Return the reference object for interfacing use
        return layerRef;
    }

    template<typename _Layer>
    LayerReference<_Layer> Protocol::addLayer(internal::role_receiver_t) {
        // First create the new layer on the heap (so it can be of type _Layer) with the receiver tag. If the layer
        // has different behaviour depending on its role, it can distinguish its role through having a different
        // constructor for the sender and receiver tags
        std::unique_ptr<_Layer> layer(new _Layer(Receiver));
        // Construct a layer reference object - this takes the reference to the object itself and also
        // the current layer index (which is then incremented)
        LayerReference<_Layer> layerRef(*layer, currentLayerIndex++);

        // Add the pointer to the back of the layers vector by moving (so we are not duplicating memory
        // and have only a single reference to that piece of memory)
        layers.push_back(std::move(layer));

        // Return the reference object for interfacing use
        return layerRef;
    }

    template<typename _From, typename _To>
    void Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom,
                        const LayerReference<typename _To::LayerType> &layerTo) {
        // Make the static type assertion that the connection we are trying to make is of the same type - it is
        // nonsensical to try to link slots of different types
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type. You must specify a transform "
                      "function to connect parameters of different types.");

        // Get a reference to each of the layers for the lambda function
        typename _From::LayerType &from = layerFrom.__ref;
        typename _To::LayerType &to = layerTo.__ref;

        // Add the lambda to the link set
        links.emplace(layerFrom.__index, [&from, &to]() {
            // Read from the "from" layer at the parameter _From, then feed this returned value into the "to" layer
            // at the parameter _To
            to.template param<_To>().feed(from.template param<_From>().read());
        });
    }

    template<typename _From, typename _To>
    void
    Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom,
                   const LayerReference<typename _To::LayerType> &layerTo,
                   const std::function<const typename _To::ValueType &(const typename _From::ValueType &)> &transform) {
        // Get a reference to each of the layers for the lambda function
        typename _From::LayerType &from = layerFrom.__ref;
        typename _To::LayerType &to = layerTo.__ref;

        // Add the lambda to the link set
        links.emplace(layerFrom.__index, [&from, &to, transform]() {
            // Read from the "from" layer at the parameter _From, then feed this value through the transform function
            // and into the "to" layer at parameter _To
            to.template param<_To>().feed(transform(from.template param<_From>().read()));
        });
    }

    template<typename _From, typename _To>
    void Protocol::link(internal::input_layer_t, const LayerReference<typename _To::LayerType> &layerTo) {
        // Make the static type assertion that the connection we are trying to make is of the same type - it is
        // nonsensical to try to link slots of different types
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type.");

        // Get a reference to the "to" layer for the lambda function
        typename _To::LayerType &to = layerTo.__ref;

        // Add a new entry to the list of feeds for the specific slot index. This allows for a one-to-many
        // mapping
        feeds[_From::paramIndex()].emplace_back(
                [&to](internal::ParameterValue value) {
                    // Feed the passed in value to the "to" layer directly at the _To slot
                    to.template param<_To>().feed(value.get<typename _From::ValueType>());
                }
        );
    }

    template<typename _From, typename _To>
    void Protocol::link(internal::input_layer_t, const LayerReference<typename _To::LayerType> &layerTo,
                        const std::function<const typename _To::ValueType &(
                                const typename _From::ValueType &)> &transform) {
        // Get a reference to the "to" layer for the lambda function
        typename _To::LayerType &to = layerTo.__ref;

        // Add a new entry to the list of feeds for the specific slot index. This allows for a one-to-many
        // mapping
        feeds[_From::paramIndex()].emplace_back(
                [&to, transform](internal::ParameterValue value) {
                    // Feed the passed in value to the "to" layer directly at the _To slot
                    to.template param<_To>().feed(transform(value.get<typename _From::ValueType>()));
                }
        );
    }

    template<typename _From, typename _To>
    void Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom, internal::output_layer_t) {
        // Make the static type assertion that the connection we are trying to make is of the same type - it is
        // nonsensical to try to link slots of different types
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type.");

        // Get a reference to the "from" layer for the lambda function
        typename _From::LayerType &from = layerFrom.__ref;

        // Add an entry to the outputs map at the index of the specified output parameter.
        outputs.emplace(
                _To::paramIndex(),
                [&from]() {
                    // Return the parameter value from the "from" layer at the _From slot
                    return from.template param<_From>().read();
                }
        );
    }

    template<typename _From, typename _To>
    void Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom, internal::output_layer_t,
                        std::function<const typename _To::ValueType &(const typename _From::ValueType &)> &transform) {
        // Get a reference to the "from" layer for the lambda function
        typename _From::LayerType &from = layerFrom.__ref;

        // Add an entry to the outputs map at the index of the specified output parameter.
        outputs.emplace(
                _To::paramIndex(),
                [&from, transform]() {
                    // Return the parameter value from the "from" layer at the _From slot
                    return transform(from.template param<_From>().read());
                }
        );
    }

    template<typename _To, typename... _From>
    void Protocol::multiLink(typename internal::InputOrLayerSwitch<typename _From::LayerType>::Type ...layersFrom,
                             const LayerReference<typename _To::LayerType> &layerTo,
                             const std::function<const typename _To::ValueType &(const typename _From::ValueType &...)> &transform) {
        // Get a reference to the layer for the lambda function
        typename _To::LayerType &to = layerTo.__ref;

        parameterGroups.push_back(internal::ParameterGroup::create<typename _From::ValueType...>());
        internal::ParameterGroup &group = parameterGroups.back();

        addMultiLinks<_To, _From...>(0, group, layersFrom..., to, [transform](internal::ParameterGroup &parameterGroup) {
            return internal::__transformerExpander<typename _To::ValueType, _From...>(
                    transform, parameterGroup, std::make_integer_sequence<size_t, sizeof...(_From)>{}
            );
        });
    }

    template<typename _Param>
    void Protocol::feed(const typename _Param::ValueType &value) {
        // Loop over each feed function for this slot
        for (const std::function<void(internal::ParameterValue)> &feeder : feeds[_Param::paramIndex()]) {
            // Call the feeder with the given value. This has to be cast to a ParameterValue wrapper type
            // as we cannot store a set of feed functions which take arbitrary types as this cannot
            // be known at compile time.
            feeder(internal::ParameterValue(value));
        }
    }

    template<typename _Param>
    typename _Param::ValueType Protocol::read() {
        // Get the reader function for the given slot and cast it to the desired value type through the ParameterValue
        // get<> interface
        return outputs[_Param::paramIndex()]().template get<typename _Param::ValueType>();
    }

    template<typename _To, typename _From, typename... _Rest>
    void Protocol::addMultiLinks(size_t index, internal::ParameterGroup &parameterGroup,
                                  typename std::enable_if<std::is_base_of_v<internal::ProtocolLayer, typename _From::LayerType>,
                                          const LayerReference<typename _From::LayerType> &>::type layerFrom,
                                  typename internal::InputOrLayerSwitch<_Rest>::Type ...restFrom,
                                  typename _To::LayerType &layerTo,
                                  const std::function<const typename _To::ValueType &(internal::ParameterGroup &)> &transform) {
        // Get a reference to the from for the lambda function
        typename _From::LayerType &from = layerFrom.__ref;

        // Add the lambda to the link set
        links.emplace(layerFrom.__index, [&from, &layerTo, transform, &parameterGroup, index]() {
            // Set the parameter at the correct index in the parameter group to the correct from parameter
            parameterGroup.set(index, from.template param<_From>().read());
            // If the parameter group is completely filled, transform the group and pass the result into
            // the target slot
            if (parameterGroup.ready()) {
                layerTo.template param<_To>().feed(transform(parameterGroup));
            }
        });

        // Call the recursive case with the tail and incremented index
        addMultiLinks<_To, _Rest...>(index + 1, parameterGroup, restFrom..., layerTo, transform);
    }

    template<typename _To, typename _From, typename... _Rest>
    void Protocol::addMultiLinks(size_t index, internal::ParameterGroup &parameterGroup,
                                  typename std::enable_if<!std::is_base_of_v<internal::ProtocolLayer, typename _From::LayerType>,
                                          internal::input_layer_t>::type layerFrom,
                                  typename internal::InputOrLayerSwitch<_Rest>::Type ...restFrom,
                                  typename _To::LayerType &layerTo,
                                  const std::function<const typename _To::ValueType &(internal::ParameterGroup &)> &transform) {

        // Add a new entry to the list of feeds for the specific slot index. This allows for a one-to-many
        // mapping
        feeds[_From::paramIndex()].emplace_back(
                [&layerTo, &parameterGroup, transform, index](internal::ParameterValue value) {
                    // Set the parameter at the correct index in the parameter group to the correct from parameter
                    parameterGroup.set(index, value.get<typename _From::ValueType>());
                    // If the parameter group is completely filled, transform the group and pass the result into
                    // the target slot
                    if (parameterGroup.ready()) {
                        layerTo.template param<_To>().feed(transform(parameterGroup));
                    }
                }
        );

        // Call the recursive case with the tail and incremented index
        addMultiLinks<_To, _Rest...>(index + 1, parameterGroup, restFrom..., layerTo, transform);
    }

}

#endif //CONTRACTS_SITE_CLIENT_PROTOCOL_H
