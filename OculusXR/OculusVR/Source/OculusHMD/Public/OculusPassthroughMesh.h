#pragma once

#include "CoreMinimal.h"

namespace OculusHMD
{

class FOculusPassthroughMesh : public FRefCountedObject
{
public:
	FOculusPassthroughMesh(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles)
	:	Vertices(InVertices)
	,	Triangles(InTriangles)
	{
	}

	const TArray<FVector>& GetVertices() const  { return Vertices; };
	const TArray<int32>& GetTriangles() const { return Triangles; };

private:
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
};

typedef TRefCountPtr<FOculusPassthroughMesh> FOculusPassthroughMeshRef;

} // namespace OculusHMD

