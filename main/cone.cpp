#include "cone.hpp"

SimpleMeshData make_cone( bool aCapped, std::size_t aSubdivs, Vec3f aColor, Mat44f aPreTransform )
{
	std::vector<Vec3f> pos;
	std::vector<Vec3f> normals;

	float prevY = std::cos(0.f);
	float prevZ = std::sin(0.f);

	Vec3f capCenter = Vec3f{ 0.f, 0.f, 0.f }; 

	if (aCapped)
	{
		const float fullCircle = 2.f * 3.1415926535897932384626433832795f;
		for (std::size_t i = 0; i < aSubdivs; ++i)
		{
			float const angle = (i + 1) / static_cast<float>(aSubdivs) * fullCircle;
			float y = std::cos(angle); // cur y
			float z = std::sin(angle); // cur z

			Vec3f normalSide = normalize(Vec3f{ 1.f, y, z });

			// basic
			pos.insert(pos.end(), {
				Vec3f{ 0.f, prevY, prevZ },
				Vec3f{ 0.f, y, z },
				Vec3f{ 1.f, 0.f, 0.f }
				});
			normals.insert(normals.end(), { normalSide, normalSide, normalSide });

			// front triangle
			pos.insert(pos.end(), {
				Vec3f{ 0.f, 0.f, 0.f },
				Vec3f{ 0.f, prevY, prevZ },
				Vec3f{ 0.f, y, z }
				});
			normals.insert(normals.end(), { normalSide, normalSide, normalSide });

			// inside and outside triangles
			for (int flip = 0; flip < 2; ++flip)
			{
				pos.insert(pos.end(), {
					capCenter,
					Vec3f{ 0.f, flip == 0 ? y : prevY, flip == 0 ? z : prevZ },
					Vec3f{ 0.f, flip == 0 ? prevY : y, flip == 0 ? prevZ : z }
					});
				normals.insert(normals.end(), { normalSide, normalSide, normalSide });
			}

			prevY = y;
			prevZ = z;
		}
	}
	else
	{
		const float fullCircle = 2.f * 3.1415926535897932384626433832795f; // 2��
		for (std::size_t i = 0; i < aSubdivs; ++i)
		{
			// cur angle
			float const angle = static_cast<float>(i + 1) / static_cast<float>(aSubdivs) * fullCircle;

			float y = std::cos(angle); // new y
			float z = std::sin(angle); // new z

			Vec3f normalSide = normalize(Vec3f{ 1.f, y, z });

			// first triangle
			pos.insert(pos.end(), {
				Vec3f{ 0.f, prevY, prevZ },
				Vec3f{ 0.f, y, z },
				Vec3f{ 1.f, 0.f, 0.f }
				});
			normals.insert(normals.end(), { normalSide, normalSide, normalSide });

			// front
			pos.insert(pos.end(), {
				Vec3f{ 0.f, 0.f, 0.f },  // middle
				Vec3f{ 0.f, prevY, prevZ },
				Vec3f{ 0.f, y, z }
				});
			normals.insert(normals.end(), { normalSide, normalSide, normalSide });

			prevY = y;
			prevZ = z;
		}
	}
		
	//transformation of all positions
	for (size_t i = 0; i < pos.size(); ++i) {
		// Apply pre-transform and perspective division for position
		Vec4f t = aPreTransform * Vec4f{ pos[i].x, pos[i].y, pos[i].z, 1.f };
		pos[i] = Vec3f{ t.x / t.w, t.y / t.w, t.z / t.w };

		// Apply pre-transform for normals (ignore w for normals)
		Vec4f tn = aPreTransform * Vec4f{ normals[i].x, normals[i].y, normals[i].z, 0.f };
		normals[i] = normalize(Vec3f{ tn.x, tn.y, tn.z });
	}

	SimpleMeshData cone; 
	cone.positions = pos; 
	cone.colors = std::vector<Vec3f>(pos.size(), aColor); 
	cone.normals = normals; 

	return cone; 
}

