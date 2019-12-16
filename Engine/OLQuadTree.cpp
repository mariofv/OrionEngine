#include "OLQuadTree.h"

#include <algorithm>

OLQuadTree::OLQuadTree()
{
}

OLQuadTree::~OLQuadTree()
{
	Clear();
}

void OLQuadTree::Create(AABB2D limits)
{
	Clear();
	root = new OLQuadTreeNode(limits);
	flattened_tree.push_back(root);
}

void OLQuadTree::Clear()
{
	for (auto & node : flattened_tree)
	{
		delete node;
	}
	flattened_tree.clear();
	root = nullptr;
}

void OLQuadTree::Insert(GameObject &game_object)
{
	if (!game_object.IsStatic())
	{
		return;
	}
	assert(root->box.Intersects(game_object.aabb.bounding_box2D));

	std::vector<OLQuadTreeNode*> intersecting_leaves;
	FindLeaves(game_object.aabb.bounding_box2D, intersecting_leaves);
	assert(intersecting_leaves.size() > 0);

	bool reinsert = false;
	for (auto &leaf : intersecting_leaves)
	{
		if (leaf->depth == max_depth)
		{
			leaf->InsertGameObject(&game_object);
		}
		else 
		{
			if (leaf->objects.size() == bucket_size)
			{
				std::vector<OLQuadTreeNode*> generated_nodes;
				leaf->Split(generated_nodes);
				flattened_tree.insert(flattened_tree.end(), generated_nodes.begin(), generated_nodes.end());
				reinsert = true;

			}
			else
			{
				leaf->InsertGameObject(&game_object);
			}
		}
	}

	if (reinsert)
	{
		Insert(game_object);
	}
}

void OLQuadTree::CollectIntersect(std::vector<GameObject*> &game_objects, const ComponentCamera &camera)
{
	root->CollectIntersect(game_objects, camera);
	auto it = std::unique(game_objects.begin(), game_objects.end());
	game_objects.erase(it, game_objects.end());
}

void OLQuadTree::FindLeaves(const AABB2D &game_object_aabb, std::vector<OLQuadTreeNode*> &leaves) const
{
	for (auto &node : flattened_tree)
	{
		if (node->IsLeaf() && node->box.Intersects(game_object_aabb))
		{
			leaves.push_back(node);
		}
	}
}


std::vector<float> OLQuadTree::GetVertices(const AABB2D &box)
{
	static const int num_of_vertices = 4;
	float3 tmp_vertices[num_of_vertices];
	float2 max_point = box.maxPoint;
	float2 min_point = box.minPoint;
	//ClockWise from Top left
	tmp_vertices[0] = float3(min_point.x, max_point.y, 0.0f); // 0
	tmp_vertices[1] = float3(max_point, 0.0f); // 1
	tmp_vertices[2] = float3(max_point.x, min_point.y, 0.0f); // 2
	tmp_vertices[3] = float3(min_point, 0.0f); // 3

	std::vector<float> vertices(num_of_vertices * 3);
	for (unsigned int i = 0; i < num_of_vertices; ++i)
	{
		vertices[i * 3] = tmp_vertices[i].x;
		vertices[i * 3 + 1] = tmp_vertices[i].z;
		vertices[i * 3 + 2] = tmp_vertices[i].y;
	}

	return vertices;
}
