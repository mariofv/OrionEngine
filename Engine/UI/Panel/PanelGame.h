#ifndef _PANELGAME_H_
#define _PANELGAME_H_

#include "Panel.h"
#include <MathGeoLib.h>


class PanelGame : public Panel
{
public:
	PanelGame();
	~PanelGame();

	void Render() override;

private:
	void ShowEmptyGameWindow() const;

};

#endif //_PANELGAME_H_