//
// Created by Matthew.Sirman on 25/08/2020.
//

#include "../../../include/networking/protocol/Protocol.h"

using namespace networking;

Protocol::Protocol(Protocol &&protocol) noexcept
        : layers(std::move(protocol.layers)),
          feeds(std::move(protocol.feeds)),
          links(std::move(protocol.links)),
          outputs(std::move(protocol.outputs)),
          currentLayerIndex(protocol.currentLayerIndex) {

}

Protocol &Protocol::operator=(Protocol &&other) noexcept {
    // If we are assignment to ourself, return
    if (this == &other) {
        return *this;
    }

    // Move across each field
    this->layers = std::move(other.layers);
    this->feeds = std::move(other.feeds);
    this->links = std::move(other.links);
    this->outputs = std::move(other.outputs);
    this->currentLayerIndex = other.currentLayerIndex;

    return *this;
}

void Protocol::execute() {
    // Initialise the current layer to 0
    size_t currentLayer = 0;

    // Loop over every linker function
    for (const LinkElement &link : links) {
        // Get the layer of the current linker function - this is the index of the "from" side of the feed
        size_t layerID = link.first;
        // Get the actual linker function
        std::function < void() > linker = link.second;

        // If the layer of this link is greater than the current layer, this means that we have executed all of
        // the linker functions up to layerID. Therefore, we need to activate each layer up to
        // layerID. The goal is to activate each layer once and only once, in order, and only after it has been
        // fed all of its inputs. As there is no cyclic feeding, we know that if we have no more links which come
        // from a layer earlier than layerID, then each layer is ready for activation.
        if (layerID >= currentLayer) {
            // Activate each layer from the current layer up to layerID (exclusive)
            for (size_t l = currentLayer; l <= layerID; l++) {
                layers[l]->activate();
            }
            // Update the current layer to layer ID + 1
            currentLayer = layerID + 1;
        }

        // Call the linker function
        linker();
    }

    // We can see that every layer up to (but not including) currentLayer has been activated from above. Therefore,
    // we still need to activate the rest of the layers, so for each layer from currentLayer to the end of the layers
    // list, we activate it.
    for (size_t l = currentLayer; l < layers.size(); l++) {
        layers[l]->activate();
    }
}

void Protocol::clearData() {
    std::for_each(parameterGroups.begin(), parameterGroups.end(), [](internal::ParameterGroup &group) { group.clear(); });
}

bool Protocol::LinkComparator::operator()(const LinkElement &lhs,
                                          const LinkElement &rhs) const {
    // Compare the two link elements by their (from) layer indices
    return lhs.first < rhs.first;
}
