#pragma once

#include "memory"
#include "mutex"

class FFTData {

public:

	struct OUTPUT {
		float* data;
		int len;
	};

private:

	OUTPUT* spectreDataN;
	OUTPUT* shadowSpectreDataN;

	OUTPUT* waterfallDataN;
	OUTPUT* shadowWaterfallDataN;

	std::mutex mutex;

public:

	FFTData(int startSpectreLen);

	void init(int startSpectreLen);

	void setData(float* spectreData, float* waterfallData, int len);

	OUTPUT* getDataCopy(bool waterfall);
	OUTPUT* getDataCopy(int startPos, int len, bool waterfall);
	OUTPUT* getDataCopy(OUTPUT* data, int startPos, int len);

	void destroyData(OUTPUT* data);

};