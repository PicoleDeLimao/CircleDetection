#include "houghcell.h"

#include <iostream>

HoughCell::HoughCell(const cv::Rect2f &maxExtension, const cv::Point2f &center, float size, short numAngles, short minNumAngles)
	: mMaxExtension(maxExtension)
	, mCenter(center)
	, mSize(size)
	, mNumAngles(numAngles)
	, mMinNumAngles(minNumAngles)
	, mVisited(false)
	, mDepth(0)
	, mNumAccumulators((size_t)std::roundf(maxExtension.width / 2 / size))
{
	for (size_t i = 0; i < 4; i++)
	{
		mChildren[i] = NULL;
	}
	mAccumulators = new HoughAccumulator*[mNumAccumulators];
	for (size_t i = 0; i < mNumAccumulators; i++)
	{
		mAccumulators[i] = NULL;
	}
}

HoughCell::~HoughCell()
{
	if (mAccumulators != NULL)
	{
		for (size_t i = 0; i < mNumAccumulators; i++)
		{
			if (mAccumulators[i] != NULL)
				delete mAccumulators[i];
		}
		delete[] mAccumulators;
	}
	for (size_t i = 0; i < 4; i++)
	{
		if (mChildren[i] != NULL)
			delete mChildren[i];
	}
}
	
void HoughCell::addIntersectionsToChildren(std::set<HoughAccumulator*> &accumulators)
{
	#ifdef _BENCHMARK
		auto begin = std::chrono::high_resolution_clock::now();
	#endif
	for (size_t i = 0; i < mNumAccumulators; i++)
	{
		if (mAccumulators[i] != NULL)
		{
			for (const Intersection &intersection : mAccumulators[i]->intersections())
			{
				addIntersection(intersection, accumulators);
			}
		}
	}
	/*for (size_t i = 0; i < mNumAccumulators; i++)
	{
		if (mAccumulators[i] != NULL)
			delete mAccumulators[i];
	}
	delete[] mAccumulators;
	mAccumulators = NULL;*/
	#ifdef _BENCHMARK
		auto end = std::chrono::high_resolution_clock::now();
		gTimeAddIntersectionsChildren += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
	#endif 
}

HoughAccumulator* HoughCell::accumulate(const Intersection &intersection)
{
	//intersection.sampler->removePoint(intersection.p1);
	//intersection.sampler->removePoint(intersection.p2);
	size_t radius = std::min(mNumAccumulators - 1, (size_t)(intersection.dist / mSize));
	if (mAccumulators[radius] == NULL)
	{
		mAccumulators[radius] = new HoughAccumulator(this, radius * mSize + mSize / 2);
	}
	if (!mAccumulators[radius]->isVisited())
	{
		mAccumulators[radius]->accumulate(intersection);
		return mAccumulators[radius];
	}
	return NULL;
}

void HoughCell::addIntersection(const Intersection &intersection, std::set<HoughAccumulator*> &accumulators)
{
	if (!mVisited)
	{
		HoughAccumulator *accumulator = accumulate(intersection);
		if (accumulator != NULL)
			accumulators.insert(accumulator);
	}
	else
	{
		size_t childIndex = ((intersection.position.x > mCenter.x) << 1) | (intersection.position.y > mCenter.y);
		if (mChildren[childIndex] == NULL)
		{
			float newSize = mSize / 2;
			cv::Point2f center;
			switch (childIndex)
			{
				case 0:
					center = cv::Point2f(mCenter.x - newSize, mCenter.y - newSize);
					break;
				case 1:
					center = cv::Point2f(mCenter.x - newSize, mCenter.y + newSize);
					break;
				case 2:
					center = cv::Point2f(mCenter.x + newSize, mCenter.y - newSize);
					break;
				case 3:
					center = cv::Point2f(mCenter.x + newSize, mCenter.y + newSize);
			}
			mChildren[childIndex] = new HoughCell(mMaxExtension, center, newSize, mNumAngles, mMinNumAngles);
			mChildren[childIndex]->mDepth = mDepth + 1;
		}
		mChildren[childIndex]->addIntersection(intersection, accumulators);
	}
}
	
