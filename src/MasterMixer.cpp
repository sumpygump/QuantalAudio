#include "QuantalAudio.hpp"

struct MasterMixer : Module {
    enum ParamIds {
        MIX_LVL_PARAM,
        MONO_PARAM,
        ENUMS(LVL_PARAM, 2),
        NUM_PARAMS
    };
    enum InputIds {
        MIX_CV_INPUT,
        ENUMS(CH_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {
        MIX_OUTPUT,
        MIX_OUTPUT_2,
        ENUMS(CH_OUTPUT, 2),
        NUM_OUTPUTS
    };

    MasterMixer() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        configParam(MIX_LVL_PARAM, 0.0f, 2.0f, 1.0f);
        configParam(MONO_PARAM, 0.0f, 1.0f, 1.0f);
        configParam(LVL_PARAM + 0, 0.0f, 1.0f, 1.0f);
        configParam(LVL_PARAM + 1, 0.0f, 1.0f, 1.0f);
    }

    void process(const ProcessArgs &args) override {
        float mix = 0.f;
        for (int i = 0; i < 2; i++) {
            float ch = inputs[CH_INPUT + i].getVoltage();
            ch *= powf(params[LVL_PARAM + i].getValue(), 2.f);
            outputs[CH_OUTPUT + i].setVoltage(ch);
            mix += ch;
        }
        mix *= params[MIX_LVL_PARAM].getValue();

        bool is_mono = (params[MONO_PARAM].getValue() > 0.0f);

        float mix_cv = 1.f;
        if (inputs[MIX_CV_INPUT].isConnected())
            mix_cv = clamp(inputs[MIX_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        if (!is_mono && (inputs[CH_INPUT + 0].isConnected() && inputs[CH_INPUT + 1].isConnected())) {
            // If the ch 2 jack is active use stereo mode
            float attenuate = params[MIX_LVL_PARAM].getValue() * mix_cv;
            outputs[MIX_OUTPUT].setVoltage(outputs[CH_OUTPUT + 0].value * attenuate);
            outputs[MIX_OUTPUT_2].setVoltage(outputs[CH_OUTPUT + 1].value * attenuate);
        } else {
            // Otherwise use mono->stereo mode
            mix *= mix_cv;
            outputs[MIX_OUTPUT].setVoltage(mix);
            outputs[MIX_OUTPUT_2].setVoltage(mix);
        }
    }
};

struct MasterMixerWidget : ModuleWidget {
    MasterMixerWidget(MasterMixer *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MasterMixer.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Level & CV
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 2.5 - (38.0/2), 52.0), module, MasterMixer::MIX_LVL_PARAM));
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH * 2.5 - (25.0/2), 96.0), module, MasterMixer::MIX_CV_INPUT));

        // Mono/stereo switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH * 2.5 - 7.0, 162.0), module, MasterMixer::MONO_PARAM));

        // LEDs
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH * 2.5 - (21.0 + 7.0), 130.4), module, MasterMixer::LVL_PARAM + 0));
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH * 2.5 + 7.0, 130.4), module, MasterMixer::LVL_PARAM + 1));

        // Channel inputs
        addInput(createInput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) - (25.0 + 5.0), 232.0), module, MasterMixer::CH_INPUT + 0));
        addInput(createInput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) + 5.0, 232.0), module, MasterMixer::CH_INPUT + 1));

        // Channel outputs
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) - (25.0 + 5.0), 276.0), module, MasterMixer::CH_OUTPUT + 0));
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) + 5.0, 276.0), module, MasterMixer::CH_OUTPUT + 1));

        // Mix outputs
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) - (25.0 + 5.0), 320.0), module, MasterMixer::MIX_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5) + 5.0, 320.0), module, MasterMixer::MIX_OUTPUT_2));
    }
};

Model *modelMasterMixer = createModel<MasterMixer, MasterMixerWidget>("Mixer2");
