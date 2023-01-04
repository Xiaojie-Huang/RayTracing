#include "Renderer.h"

#include "Walnut/Random.h"

namespace Utils
{
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
}


void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		//不需要重新适应尺寸
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	Ray ray;
	ray.Origin = camera.GetPosition();

	//先y再x，充分利用cache
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			ray.Direction = camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()];

			glm::vec4 color = TraceRay(scene,ray);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y*m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);

}

glm::vec4 Renderer::TraceRay(const Scene& scene, const Ray& ray)
{

	//(bx^2+by^2)t^2 + 2(axbx+ayby)t + (ax^2+ay^2-r^2) = 0
	//where
	//a = ray origin
	//b = ray direction
	//r = radius
	//t = hit distance
	//原点是0,0  所以实际上屏幕坐标就可以认为从原点到该点屏幕的射线的方向向量
	
	if (scene.Spheres.size() == 0)
		return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	const Sphere* ClosestSphere = nullptr;
	float hitDistance = FLT_MAX;
	for (const Sphere& sphere : scene.Spheres)
	{
		glm::vec3 origin = ray.Origin - sphere.Position;

		//一元二次方程系数
		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		//求根公式正负
		//b^2 -4ac
		float discriminant = b * b - 4.0f * a * c;

		if (discriminant < 0)
			continue;

		//计算碰撞相交点
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT < hitDistance)
		{
			hitDistance = closestT;
			ClosestSphere = &sphere;
		}

	}

	if (ClosestSphere == nullptr)
		return glm::vec4(0.0f,0.0f,0.0f,1.0f);

	glm::vec3 origin = ray.Origin - ClosestSphere->Position;

	glm::vec3 hitPoint = origin + ray.Direction * hitDistance;
	glm::vec3 normal = glm::normalize(hitPoint);

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));

	float d = glm::max(glm::dot(normal,-lightDir), 0.0f);

	glm::vec3 sphereColor = ClosestSphere->Albedo;
	sphereColor *= d;

	return glm::vec4(sphereColor, 1.0f);
}
