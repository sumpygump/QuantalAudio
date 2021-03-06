#include "QuantalAudio.hpp"

struct DaisyChannel : Module {
    enum ParamIds {
        CH_LVL_PARAM,
        MUTE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH_INPUT,
        LVL_CV_INPUT,
        CHAIN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH_OUTPUT,
        CHAIN_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        NUM_LIGHTS
    };

    // Hypothetically the max number of channels that could be chained
    // Needs to match the divisor in the daisy master class
    float DAISY_DIVISOR = 16.f;
    bool muted = false;
    dsp::SchmittTrigger muteTrigger;

    DaisyChannel() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CH_LVL_PARAM, 0.0f, 1.0f, 1.0f);
        configParam(MUTE_PARAM, 0.0f, 1.0f, 0.0f);
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        // mute
        json_object_set_new(rootJ, "muted", json_boolean(muted));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        // mute
        json_t *mutedJ = json_object_get(rootJ, "muted");
        if (mutedJ)
            muted = json_is_true(mutedJ);
    }

    void process(const ProcessArgs &args) override {
        if (muteTrigger.process(params[MUTE_PARAM].getValue())) {
            muted = !muted;
        }

        float ch = 0.f;
        if (!muted) {
            ch = inputs[CH_INPUT].getVoltage();
            ch *= powf(params[CH_LVL_PARAM].getValue(), 2.f);

            if (inputs[LVL_CV_INPUT].isConnected()) {
                float _cv = clamp(inputs[LVL_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
                ch *= _cv;
            }
        }

        outputs[CH_OUTPUT].setVoltage(ch);

        // Make the voltage small to the chain by dividing by the divisor;
        outputs[CHAIN_OUTPUT].setVoltage(inputs[CHAIN_INPUT].getVoltage() + ch / DAISY_DIVISOR);
        lights[MUTE_LIGHT].value = (muted);
    }
};

struct DaisyChannelWidget : ModuleWidget {
    DaisyChannelWidget(DaisyChannel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DaisyChannel.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Channel Input/Output
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), module, DaisyChannel::CH_INPUT));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 245.0), module, DaisyChannel::CH_OUTPUT));

        // Level & CV
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH - 10.5, 121.4), module, DaisyChannel::CH_LVL_PARAM));
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 89.0), module, DaisyChannel::LVL_CV_INPUT));

        // Mute
        addParam(createParam<LEDButton>(Vec(RACK_GRID_WIDTH - 9.0, 206.0), module, DaisyChannel::MUTE_PARAM));
        addChild(createLight<MediumLight<RedLight>>(Vec(RACK_GRID_WIDTH - 4.5, 210.25f), module, DaisyChannel::MUTE_LIGHT));

        // Chain Input/Output
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 290.5), module, DaisyChannel::CHAIN_INPUT));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), module, DaisyChannel::CHAIN_OUTPUT));
    }
};

Model *modelDaisyChannel = createModel<DaisyChannel, DaisyChannelWidget>("DaisyChannel");
