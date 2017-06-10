#include "stdafx.h"
#include "FrameStat.h"


namespace zeta
{

	FrameStat::FrameStat()
	{
		frame_time_ = 0;
		frame_count_ = 0;
	}
	FrameStat::~FrameStat()
	{

	}

	FrameStat& FrameStat::Instance()
	{
		static FrameStat obj;
		return obj;
	}

}