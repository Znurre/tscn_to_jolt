#include <fstream>
#include <numeric>
#include <variant>

#include <gd_parser/gd_parser.hpp>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <CLI/App.hpp>

template <typename T>
struct extractor
{
	using result = std::optional<T>;

	std::optional<T> visit(const T& value)
	{
		return value;
	}

	std::optional<T> visit(auto)
	{
		return {};
	}
};

template <typename T>
std::optional<T> read_field(const std::vector<gd::field>& fields, const std::string& name)
{
	auto field = std::ranges::find_if(fields, [&](auto& field) {
		return field.name == name;
	});

	if (field == end(fields))
	{
		return {};
	}

	return havoc::visit(extractor<T> {}, field->value);
}

std::optional<gd::tag> find_ancestor(gd::tag& node, std::unordered_map<std::string, gd::tag>& nodes)
{
	if (auto parent_id = read_field<std::string>(node.fields, "parent"); !parent_id)
	{
		spdlog::error("Node does not have a 'parent' field");

		return {};
	}
	else if (parent_id == ".")
	{
		return node;
	}
	else if (auto parent_it = nodes.find(*parent_id); parent_it == end(nodes))
	{
		spdlog::error("Parent with id '{}' not found", *parent_id);

		return {};
	}
	else
	{
		return find_ancestor(parent_it->second, nodes);
	}
}

std::vector<float> get_values(const gd::constructable& constructable)
{
	std::vector<float> values;

	for (auto& argument : constructable.arguments)
	{
		if (auto numeric = havoc::visit(extractor<gd::numeric_t> {}, argument); !numeric)
		{
			return {};
		}
		else
		{
			auto value = std::visit(
				[](auto value) -> float {
					return value;
				},
				*numeric
			);

			values.push_back(value);
		}
	}

	return values;
}

int main(int argc, char** argv)
{
	JPH::RegisterDefaultAllocator();

	std::string input_filename;
	std::string output_filename;

	CLI::App app("TSCN to Jolt");

	app.failure_message(CLI::FailureMessage::help);
	app.add_option("input_filename", input_filename)->required();
	app.add_option("output_filename", output_filename)->required();

	CLI11_PARSE(app, argc, argv);

	std::ifstream input(input_filename);
	std::ofstream output(output_filename, std::ios::out | std::ios::binary);

	spdlog::info("Parsing file...");

	auto file = gd::parse(input);

	std::unordered_map<std::string, gd::tag> nodes;
	std::unordered_map<std::string, gd::tag> sub_resources;

	JPH::StreamOutWrapper output_wrapper(output);

	spdlog::info("Preparing data...");

	for (auto& tag : file.tags)
	{
		if (tag.identifier == "node")
		{
			auto identifiers = {
				read_field<std::string>(tag.fields, "parent"),
				read_field<std::string>(tag.fields, "name"),
			};

			auto path = std::accumulate(begin(identifiers), end(identifiers), std::string(), [](auto path, auto id) {
				if (!id || id == ".")
				{
					return path;
				}

				if (empty(path))
				{
					return *id;
				}

				return std::format("{}/{}", path, *id);
			});

			if (!empty(path))
			{
				nodes[path] = tag;
			}
		}
		else if (tag.identifier == "sub_resource")
		{
			if (auto id = read_field<std::string>(tag.fields, "id"))
			{
				sub_resources[*id] = tag;
			}
		}
	}

	spdlog::info("Building shapes...");

	for (auto& [_, node] : nodes)
	{
		if (read_field<std::string>(node.fields, "type") != "CollisionShape3D")
		{
			continue;
		}

		if (auto ancestor = find_ancestor(node, nodes); !ancestor)
		{
			spdlog::error("Failed to find ancestor for node");

			continue;
		}
		else if (auto transform = read_field<gd::constructable>(ancestor->assignments, "transform"); !transform)
		{
			spdlog::error("Ancestor has no transform");

			continue;
		}
		else if (transform->identifier != "Transform3D" || size(transform->arguments) != 12)
		{
			spdlog::error(
				"Transform did not have the expected format (expected Transform3D with 12 arguments, actual {} with {} "
				"arguments)",
				transform->identifier,
				size(transform->arguments)
			);

			continue;
		}
		else if (auto shape = read_field<gd::constructable>(node.assignments, "shape"); !shape)
		{
			spdlog::error("Node does not have an associated shape");

			continue;
		}
		else if (size(shape->arguments) != 1)
		{
			spdlog::error("Shape did not have the expected format");

			continue;
		}
		else if (auto name = havoc::visit(extractor<std::string> {}, shape->arguments[0]); !name)
		{
			spdlog::error("Failed to obtain shape id");

			continue;
		}
		else if (auto resource_it = sub_resources.find(*name); resource_it == end(sub_resources))
		{
			spdlog::error("Shape id '{}' does not refer to an existing sub resource", *name);

			continue;
		}
		else if (auto data = read_field<gd::constructable>(resource_it->second.assignments, "data"); !data)
		{
			spdlog::error("Shape does not have associated data");

			continue;
		}
		else if (data->identifier != "PackedVector3Array")
		{
			spdlog::error("Data did not have the expected format (expected PackedVector3Array, actual {})", data->identifier);

			continue;
		}
		else
		{
			auto transform_values = get_values(*transform);

			if (empty(transform_values))
			{
				spdlog::error("Failed to extract transform values");

				continue;
			}

			auto vertice_values = get_values(*data);

			if (empty(vertice_values))
			{
				spdlog::error("Failed to extract vertice values");

				continue;
			}

			glm::mat4x3 transform = {
				glm::vec3 { transform_values[0], transform_values[1], transform_values[2] },
				glm::vec3 { transform_values[3], transform_values[4], transform_values[5] },
				glm::vec3 { transform_values[6], transform_values[7], transform_values[8] },
				glm::vec3 { transform_values[9], transform_values[10], transform_values[11] },
			};

			auto num_vertices = size(vertice_values);

			JPH::TriangleList mesh;

			for (auto i = 0; i < num_vertices; i += 9)
			{
				JPH::Float3 vertices[3];

				for (auto j = 0ul; j < 3; j++)
				{
					glm::vec4 vec = {
						vertice_values[(j * 3) + i + 0],
						vertice_values[(j * 3) + i + 1],
						vertice_values[(j * 3) + i + 2],
						1,
					};

					auto transformed = transform * vec;

					vertices[j].x = transformed.x;
					vertices[j].y = transformed.y;
					vertices[j].z = transformed.z;
				}

				mesh.emplace_back(vertices[2], vertices[1], vertices[0]);
			}

			JPH::MeshShapeSettings mesh_shape_settings(mesh);

			auto shape_result = mesh_shape_settings.Create();

			if (shape_result.HasError())
			{
				spdlog::error("Failed to generate collision shape: {}", shape_result.GetError());

				return 1;
			}

			JPH::Ref<JPH::Shape> mesh_shape(new JPH::MeshShape(mesh_shape_settings, shape_result));

			mesh_shape->SaveBinaryState(output_wrapper);
		}
	}

	return 0;
}
