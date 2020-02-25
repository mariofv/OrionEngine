#ifndef _PANELCONFIGURATION_H_
#define _PANELCONFIGURATION_H_

#include "Panel.h"

class PanelConfiguration : public Panel
{
public:
	PanelConfiguration();
	~PanelConfiguration() = default;

	void Render() override;

private:
	void ShowHardware() const;

	void ShowWindowOptions();
	void ShowRenderOptions();
	void ShowTimeOptions();
	void ShowInputOptions();
};

#endif //_PANELCONFIGURATION_H_