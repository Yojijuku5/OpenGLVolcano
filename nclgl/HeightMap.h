#pragma once
#include "Mesh.h"
#include <string>

class HeightMap : public Mesh {
public:
	HeightMap(const std::string& name);
	~HeightMap(void) {};

	Vector3 GetHeightMapSize() const {
		return heightmapSize;
	}

protected:
	Vector3 heightmapSize;
};

