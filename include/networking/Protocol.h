//
// Created by Matthew.Sirman on 25/08/2020.
//

#ifndef CONTRACTS_SITE_CLIENT_PROTOCOL_H
#define CONTRACTS_SITE_CLIENT_PROTOCOL_H

#include <type_traits>
#include <vector>
#include <memory>
#include <functional>

namespace networking {

    class Protocol;

    namespace internal {
        struct ProtocolLayer {
        private:
            template<typename _Param>
            constexpr _Param &param() {};
        };

        struct ParameterValue {
        public:
            template<typename _Ty>
            ParameterValue(const _Ty &value);

            template<typename _Ty>
            _Ty get() const;

            template<typename _Ty>
            void set(const _Ty &value);

        private:
            void *__data;
        };

        template<size_t _index, typename _Layer, typename _Ty>
        struct Connector {
            using LayerType = _Layer;
            using ValueType = _Ty;

            static constexpr size_t paramIndex();

            void feed(ParameterValue value);

            ParameterValue read() const;

        private:
            ValueType data;
        };

        struct input_layer_t {
            enum _Construct {
                _Token
            };

            explicit constexpr input_layer_t(_Construct) {}
        };

        struct output_layer_t {
            enum _Construct {
                _Token
            };

            explicit constexpr output_layer_t(_Construct) {}
        };

        template<typename _Ty>
        ParameterValue::ParameterValue(const _Ty &value) {
            __data = &value;
        }

        template<typename _Ty>
        _Ty ParameterValue::get() const {
            return *((_Ty *) __data);
        }

        template<typename _Ty>
        void ParameterValue::set(const _Ty &value) {
            __data = &value;
        }

        template<size_t _index, typename _Layer, typename _Ty>
        constexpr size_t Connector<_index, _Layer, _Ty>::paramIndex() {
            return _index;
        }

        template<size_t index, typename _Layer, typename _Ty>
        void Connector<index, _Layer, _Ty>::feed(ParameterValue value) {
            data = value.get<_Ty>();
        }

        template<size_t index, typename _Layer, typename _Ty>
        ParameterValue Connector<index, _Layer, _Ty>::read() const {
            return data;
        }
    }

    template<typename _Layer>
    struct LayerReference {
        friend class Protocol;
    public:

    private:
        size_t __index;
    };

    struct KeyExchange : public internal::ProtocolLayer {
        friend class Protocol;
    public:
        using ParamIntX = internal::Connector<0, KeyExchange, int>;
        using ParamIntZ = internal::Connector<1, KeyExchange, int>;
        using ParamFloatY = internal::Connector<2, KeyExchange, float>;

    private:
        ParamIntX x;
        ParamIntZ z;
        ParamFloatY y;
    };

    struct InputLayer : public internal::ProtocolLayer {
        template<typename _Param>
        void feed(const typename _Param::ValueType &value);
    };

    struct OutputLayer : public internal::ProtocolLayer {
        template<typename _Param>
        typename _Param::ValueType read();
    };

    class Protocol {
    private:

    public:
        inline static constexpr internal::input_layer_t Inputs { internal::input_layer_t::_Construct::_Token };
        inline static constexpr internal::output_layer_t Outputs { internal::output_layer_t::_Construct::_Token };

        template<typename _Layer>
        LayerReference<_Layer> addLayer();

        template<typename _Input>
        void setInput();

        template<typename _Output>
        void setOutput();

        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  const LayerReference<typename _To::LayerType> &layerTo);

        template<typename _From, typename _To>
        void link(internal::input_layer_t,
                  const LayerReference<typename _To::LayerType> &layerTo);

        template<typename _From, typename _To>
        void link(const LayerReference<typename _From::LayerType> &layerFrom,
                  internal::output_layer_t);

        template<typename _Param>
        void feed(const typename _Param::ValueType &value);

        template<typename _Param>
        typename _Param::ValueType read();

        void execute();

    private:
        std::vector<std::unique_ptr<internal::ProtocolLayer>> layers;
        std::unique_ptr<InputLayer> input;
        std::unique_ptr<OutputLayer> output;

        std::unordered_map<size_t, std::function<void(internal::ParameterValue)>> feeds;
        std::vector<std::function<void()>> links;
        std::unordered_map<size_t, std::function<internal::ParameterValue()>> outputs;
    };

    // TEMP START

    template<>
    constexpr typename KeyExchange::ParamIntX &KeyExchange::param<KeyExchange::ParamIntX>() {
        return x;
    }

    template<>
    constexpr typename KeyExchange::ParamFloatY &KeyExchange::param<KeyExchange::ParamFloatY>() {
        return y;
    }

    template<>
    constexpr typename KeyExchange::ParamIntZ &KeyExchange::param<KeyExchange::ParamIntZ>() {
        return z;
    }

    // TEMP END

    template<typename _Param>
    void InputLayer::feed(const typename _Param::ValueType &value) {

    }

    template<typename _Param>
    typename _Param::ValueType OutputLayer::read() {

    }

    template<typename _Layer>
    LayerReference<_Layer> Protocol::addLayer() {
        std::unique_ptr<_Layer> layer = std::make_unique<_Layer>();

        return LayerReference<_Layer>();
    }

    template<typename _Input>
    void Protocol::setInput() {
        input = std::make_unique<_Input>();
    }

    template<typename _Output>
    void Protocol::setOutput() {
        output = std::make_unique<_Output>();
    }

    template<typename _From, typename _To>
    void Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom,
                        const LayerReference<typename _To::LayerType> &layerTo) {
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type.");

        links.push_back([this, &layerFrom, &layerTo]() {
            layers[layerFrom.__index]->param<_From>();
        });
    }

    template<typename _From, typename _To>
    void Protocol::link(internal::input_layer_t, const LayerReference<typename _To::LayerType> &layerTo) {
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type.");
    }

    template<typename _From, typename _To>
    void Protocol::link(const LayerReference<typename _From::LayerType> &layerFrom, internal::output_layer_t) {
        static_assert(std::is_same_v<typename _From::ValueType, typename _To::ValueType>,
                      "Layer link must connect parameters of the same type.");
    }

    template<typename _Param>
    void Protocol::feed(const typename _Param::ValueType &value) {

    }

    template<typename _Param>
    typename _Param::ValueType Protocol::read() {
        return nullptr;
    }

    void test() {
        struct InputSpec : public InputLayer {
            using InputLayer::InputLayer;

            using KeyInput = internal::Connector<0, InputSpec, int>;
        };

        struct OutputSpec : public OutputLayer {
            using OutputLayer::OutputLayer;

            using IntOutput = internal::Connector<0, OutputSpec, int>;
        };

        Protocol p;

        p.setInput<InputSpec>();
        LayerReference<KeyExchange> l1 = p.addLayer<KeyExchange>();
        LayerReference<KeyExchange> l2 = p.addLayer<KeyExchange>();
        p.setOutput<OutputSpec>();

        p.link<InputSpec::KeyInput, KeyExchange::ParamIntX>(Protocol::Inputs, l1);
        p.link<KeyExchange::ParamFloatY, KeyExchange::ParamFloatY>(l1, l2);
        p.link<KeyExchange::ParamIntX, OutputSpec::IntOutput>(l2, Protocol::Outputs);

        p.feed<InputSpec::KeyInput>(5);

        p.execute();

        int outputValue = p.read<OutputSpec::IntOutput>();
    }

}

#endif //CONTRACTS_SITE_CLIENT_PROTOCOL_H
