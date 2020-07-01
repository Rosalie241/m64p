#include <QObject>
#include <stdio.h>
#include "FullscreenResolutions.h"
#include "../Config.h"
#include "../mupenplus/GLideN64_mupenplus.h"

static struct
{
	struct
	{
		uint32_t width, height, refreshRate;
	} selected;

	struct
	{
		uint32_t width, height;
	} resolution[32];

	uint32_t refreshRate[32];

	uint32_t numResolutions;
	uint32_t numRefreshRates;
} fullscreen;

#include <iostream>
static void _fillFullscreenRefreshRateList(QStringList &_listRefreshRates, int &_rateIdx)
{
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numRefreshRates = 0;
	_rateIdx = 0;

	int resolutions_length = 32;
	m64p_2d_size *resolutions = (m64p_2d_size *)malloc(resolutions_length * sizeof(m64p_2d_size));
	m64p_error ret;

	ret = CoreVideo_ListFullscreenModes(resolutions, &resolutions_length);

	if (ret != M64ERR_SUCCESS)
		return;

	for (int i = 0; i < resolutions_length; i++)
	{
		// skip unrelated resolutions
		if ((fullscreen.selected.height != resolutions[i].uiHeight) ||
			(fullscreen.selected.width != resolutions[i].uiWidth))
			continue;

		for (int x = 0; x < resolutions[i].refreshRateCount; x++)
		{
			int refreshRate = resolutions[i].refreshRates[x];

			_listRefreshRates.append(QString::number(refreshRate) + " Hz");
			fullscreen.refreshRate[fullscreen.numRefreshRates] = refreshRate;

			if (fullscreen.selected.refreshRate == refreshRate)
				_rateIdx = fullscreen.numRefreshRates;

			fullscreen.numRefreshRates++;
		}
	}

	free(resolutions);
}

void fillFullscreenResolutionsList(QStringList &_listResolutions, int &_resolutionIdx, QStringList &_listRefreshRates, int &_rateIdx)
{
	fullscreen.selected.width = config.video.fullscreenWidth;
	fullscreen.selected.height = config.video.fullscreenHeight;
	fullscreen.selected.refreshRate = config.video.fullscreenRefresh;

	memset(&fullscreen.resolution, 0, sizeof(fullscreen.resolution));
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numResolutions = 0;
	fullscreen.numRefreshRates = 0;
	_resolutionIdx = 0;

	int resolutions_length = 32;
	m64p_2d_size *resolutions = (m64p_2d_size *)malloc(resolutions_length * sizeof(m64p_2d_size));
	m64p_error ret;

	ret = CoreVideo_ListFullscreenModes(resolutions, &resolutions_length);

	if (ret != M64ERR_SUCCESS)
		return;

	for (int i = 0; i < resolutions_length; i++)
	{
		fullscreen.resolution[fullscreen.numResolutions].width = resolutions[i].uiWidth;
		fullscreen.resolution[fullscreen.numResolutions].height = resolutions[i].uiHeight;

		std::cout << resolutions[i].uiWidth << " x " << resolutions[i].uiHeight << std::endl;
		_listResolutions.append(QString::number(resolutions[i].uiWidth) + " x " + QString::number(resolutions[i].uiHeight));

		if ((fullscreen.selected.width == resolutions[i].uiWidth) &&
			(fullscreen.selected.height == resolutions[i].uiHeight))
			_resolutionIdx = fullscreen.numResolutions;

		fullscreen.numResolutions++;
	}

	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);

	free(resolutions);
}

void fillFullscreenRefreshRateList(int _resolutionIdx, QStringList &_listRefreshRates, int &_rateIdx)
{
	fullscreen.selected.width = fullscreen.resolution[_resolutionIdx].width;
	fullscreen.selected.height = fullscreen.resolution[_resolutionIdx].height;
	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);
	_rateIdx = fullscreen.numRefreshRates - 1;
}

void getFullscreenResolutions(int _idx, unsigned int &_width, unsigned int &_height)
{
	_width = fullscreen.resolution[_idx].width;
	_height = fullscreen.resolution[_idx].height;
}

void getFullscreenRefreshRate(int _idx, unsigned int &_rate)
{
	_rate = fullscreen.refreshRate[_idx];
}
