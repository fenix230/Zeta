#pragma once
#include "Utils.h"


namespace zeta
{

	class FrameStat
	{
		FrameStat();
		~FrameStat();

	public:
		static FrameStat& Instance();

		double frame_time_;
		uint64_t frame_count_;
	};

}