//***********************************************************************************************
//Expander module for Clocked, by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Pyer. 
//See ./LICENSE.md for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept and design by Marc Boulé, Nigel Sixsmith, Xavier Belmont and Steve Baker
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"


struct ClockedExpander : Module {
	enum InputIds {
		ENUMS(PW_INPUTS, 4),// fourth element not used
		ENUMS(SWING_INPUTS, 4),// fourth element not used
		NUM_INPUTS
	};


	// Expander
	float leftMessages[2][2] = {};// messages from mother (panelTheme and panelContrast)


	// No need to save, no reset
	int panelTheme;
	float panelContrast;
	unsigned int expanderRefreshCounter = 0;


	ClockedExpander() {
		config(0, NUM_INPUTS, 0, 0);
		
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		configInput(PW_INPUTS + 0, "Master clock pulse width");
		configInput(SWING_INPUTS + 0, "Master clock swing");
		for (int i = 1; i < 4; i++) {
			configInput(PW_INPUTS + i, string::f("Clock %i pulse width", i));
			configInput(SWING_INPUTS + i, string::f("Clock %i swing", i));
		}

		loadThemeAndContrastFromDefault(&panelTheme, &panelContrast);
	}


	void process(const ProcessArgs &args) override {		
		expanderRefreshCounter++;
		if (expanderRefreshCounter >= expanderRefreshStepSkips) {
			expanderRefreshCounter = 0;
			
			bool motherPresent = (leftExpander.module && leftExpander.module->model == modelClocked);
			if (motherPresent) {
				// To Mother
				float *messagesToMother = static_cast<float*>(leftExpander.module->rightExpander.producerMessage);
				for (int i = 0; i < 8; i++) {
					messagesToMother[i] = inputs[i].getVoltage();
				}
				leftExpander.module->rightExpander.messageFlipRequested = true;
				
				// From Mother
				float *messagesFromMother = static_cast<float*>(leftExpander.consumerMessage);
				panelTheme = clamp((int)(messagesFromMother[0] + 0.5f), 0, 1);			
				panelContrast = clamp(messagesFromMother[1], 0.0f, 255.0f);
			}		
		}// expanderRefreshCounter
	}// process()
};


struct ClockedExpanderWidget : ModuleWidget {
	int lastPanelTheme = -1;
	float lastPanelContrast = -1.0f;
	
	ClockedExpanderWidget(ClockedExpander *module) {
		setModule(module);
		int* mode = module ? &module->panelTheme : NULL;
		float* cont = module ? &module->panelContrast : NULL;
	
		// Main panel from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/ClockedExpander.svg")));
		SvgPanel* svgPanel = static_cast<SvgPanel*>(getPanel());
		svgPanel->fb->addChildBottom(new PanelBaseWidget(svgPanel->box.size, cont));
		svgPanel->fb->addChild(new InverterWidget(svgPanel, mode));	
		
		// Screws
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(box.size.x-30, 0), mode));
		svgPanel->fb->addChild(createDynamicScrew<IMScrew>(VecPx(box.size.x-30, 365), mode));

		// Expansion module
		static const int rowRulerExpTop = 66;
		static const int rowSpacingExp = 35;
		static const int colRulerExp = 30;
		for (int i = 0; i < 4; i++) {
			addInput(createDynamicPortCentered<IMPort>(VecPx(colRulerExp, rowRulerExpTop + rowSpacingExp * i), true, module, ClockedExpander::PW_INPUTS + i, mode));
			addInput(createDynamicPortCentered<IMPort>(VecPx(colRulerExp, 327 - rowSpacingExp * (3 - i)), true, module, ClockedExpander::SWING_INPUTS + i, mode));
		}
	}
	
	void step() override {
		if (module) {
			int panelTheme = ((static_cast<ClockedExpander*>(module))->panelTheme);
			float panelContrast = ((static_cast<ClockedExpander*>(module))->panelContrast);
			if (panelTheme != lastPanelTheme || panelContrast != lastPanelContrast) {
				SvgPanel* svgPanel = static_cast<SvgPanel*>(getPanel());
				svgPanel->fb->dirty = true;
				lastPanelTheme = panelTheme;
				lastPanelContrast = panelContrast;
			}
		}
		Widget::step();
	}
};

Model *modelClockedExpander = createModel<ClockedExpander, ClockedExpanderWidget>("Clocked-Expander");
