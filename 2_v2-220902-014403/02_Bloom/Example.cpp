#include "Example.h"

#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm> // std::clamp (C++17)

void Image::ReadFromFile(const char* filename)
{
	/*
	vcpkg install stb:x64-windows
	프로젝트 설정에서 _CRT_SECURE_NO_WARNINGS 추가 ('sprintf' in stb_image_write.h)
	#define STB_IMAGE_IMPLEMENTATION
	#include <stb_image.h>	
	#define STB_IMAGE_WRITE_IMPLEMENTATION
	#include <stb_image_write.h>
	*/

	unsigned char* img = stbi_load(filename, &width, &height, &channels, 0);

	if (width) {
		std::cout << width << " " << height << " " << channels << std::endl;
	}
	else {
		std::cout << "Error: reading " << filename << " failed." << std::endl;
	}
	// channels가 3(RGB) 또는 4(RGBA)인 경우만 가정
	// unsigned char(0에서 255)을 4채널 float(0.0f에서 1.0f)로 변환
	pixels.resize(width * height);
	for (int i = 0; i < width * height; i ++)
	{
		pixels[i].v[0] = img[i * channels] / 255.0f;
		pixels[i].v[1] = img[i * channels +1] / 255.0f;
		pixels[i].v[2] = img[i * channels +2] / 255.0f;
		pixels[i].v[3] = 1.0f;
	}

	delete [] img;
}

void Image::WritePNG(const char* filename)
{
	// 32bit에서 8bit로 변환 후에 저장

	std::vector<unsigned char> img(width * height * channels, 0);
	for (int i = 0; i < width * height; i++)
	{
		img[i * channels] = uint8_t(pixels[i].v[0] * 255.0f); // v[0]이 0.0f 이상 1.0f 이하 가정
		img[i * channels + 1] = uint8_t(pixels[i].v[1] * 255.0f);
		img[i * channels + 2] = uint8_t(pixels[i].v[2] * 255.0f);
	}

	stbi_write_png(filename, width, height, channels, img.data(), width * channels);
}

Vec4& Image::GetPixel(int i, int j)
{
	i = std::clamp(i, 0, this->width - 1);
	j = std::clamp(j, 0, this->height - 1);

	return this->pixels[i + this->width * j];
}

void Image::BoxBlur5()
{
	std::vector<Vec4> pixelsBuffer(this->pixels.size()); // 사본 복사

	/*
	* Separable convolution
	* 한 번에 2차원 Kernel을 적용하는 대신에 1차원 Kernel을 두 번 적용
	* 이해하기 쉽고 효율적이다.
	*/

	// 가로 방향 (x 방향)
#pragma omp parallel for
	for (int j = 0; j < this->height; j++)
	{
		std::cout << "병렬스레드 : " +  j << std::endl;
		for (int i = 0; i < this->width; i++)
		{
			// 주변 픽셀들의 색을 평균내어서 (i, j)에 있는 픽셀의 색을 변경
			// this->pixels로부터 읽어온 값들을 평균내어서 pixelsBuffer의 값들을 바꾸기
			Vec4 neighborColorSum{ 0.0f, 0.0f, 0.0f, 1.0f };
			for (int si = 0; si < 5; si++)
			{
				Vec4 neighborColor = this->GetPixel(i + si - 2, j);
				neighborColorSum.v[0] += neighborColor.v[0];
				neighborColorSum.v[1] += neighborColor.v[1];
				neighborColorSum.v[2] += neighborColor.v[2];
			}

			pixelsBuffer[i + this->width * j].v[0] = neighborColorSum.v[0] * 0.2f;
			pixelsBuffer[i + this->width * j].v[1] = neighborColorSum.v[1] * 0.2f;
			pixelsBuffer[i + this->width * j].v[2] = neighborColorSum.v[2] * 0.2f;
		}
	}

	// Swap
	std::swap(this->pixels, pixelsBuffer);

	//return; // 여기까지 구현하고 테스트

	// 세로 방향 (y 방향)
#pragma omp parallel for
	for (int j = 0; j < this->height; j++)
	{

		for (int i = 0; i < this->width; i++)
		{
			// 주변 픽셀들의 색을 평균내어서 (i, j)에 있는 픽셀의 색을 변경
			// this->pixels로부터 읽어온 값들을 평균내어서 pixelsBuffer의 값들을 바꾸기

			Vec4 neighborColorSum{ 0.0f, 0.0f, 0.0f, 1.0f };
			for (int si = 0; si < 5; si++)
			{
				// 가로블러 픽셀데이터 받아와서
				Vec4 neighborColor = this->GetPixel(i, j + si - 2);
				neighborColorSum.v[0] += neighborColor.v[0];
				neighborColorSum.v[1] += neighborColor.v[1];
				neighborColorSum.v[2] += neighborColor.v[2];
			}
			// 가로 + 세로 블러로 적용
			pixelsBuffer[i + this->width * j].v[0] = neighborColorSum.v[0] * 0.2f;
			pixelsBuffer[i + this->width * j].v[1] = neighborColorSum.v[1] * 0.2f;
			pixelsBuffer[i + this->width * j].v[2] = neighborColorSum.v[2] * 0.2f;
		}
	}

	// Swap
	std::swap(this->pixels, pixelsBuffer);
}


// 가중치 있는 블러처리
// 가로, 세로 필터로 2개로 분리되는 필터의 경우 5*5 의 연산을 5*1 + 1*5 연산으로 줄여줄 수 있어 연산량을 낮추는 효과를 가지게 된다
//  2차원 필터가 1차원 필터의 outer product 로 분해가 가능하다면 2개의 패스로 연산하는 것이 더욱 효율적이다
// https://en.wikipedia.org/wiki/Separable_filter
void Image::GaussianBlur5()
{
	std::vector<Vec4> pixelsBuffer(this->pixels.size());

	/*
	* 참고자료
	* https://en.wikipedia.org/wiki/Gaussian_filter
	* https://followtutorials.com/2013/03/gaussian-blurring-using-separable-kernel-in-c.html
	*/
	const float weights[5] = { 0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f };

	// 가로 방향 (x 방향)
#pragma omp parallel for
	for (int j = 0; j < this->height; j++)
	{
		for (int i = 0; i < this->width; i++)
		{
			// 주변 픽셀들의 색을 평균내어서 (i, j)에 있는 픽셀의 색을 변경
			// this->pixels로부터 읽어온 값들을 평균내어서 pixelsBuffer의 값들을 바꾸기

			Vec4 neighborColorSum{ 0.0f, 0.0f, 0.0f, 1.0f };
			for (int si = 0; si < 5; si++)
			{
				Vec4 neighborColor = this->GetPixel(i + si - 2, j);
				neighborColorSum.v[0] += neighborColor.v[0] * weights[si];
				neighborColorSum.v[1] += neighborColor.v[1] * weights[si];
				neighborColorSum.v[2] += neighborColor.v[2] * weights[si];
			}

			pixelsBuffer[i + this->width * j].v[0] = neighborColorSum.v[0];
			pixelsBuffer[i + this->width * j].v[1] = neighborColorSum.v[1];
			pixelsBuffer[i + this->width * j].v[2] = neighborColorSum.v[2];
		}
	}

	// Swap
	std::swap(this->pixels, pixelsBuffer);

	// 세로 방향 (y 방향)
#pragma omp parallel for
	for (int j = 0; j < this->height; j++)
	{
		for (int i = 0; i < this->width; i++)
		{
			// 주변 픽셀들의 색을 평균내어서 (i, j)에 있는 픽셀의 색을 변경
			// this->pixels로부터 읽어온 값들을 평균내어서 pixelsBuffer의 값들을 바꾸기

			Vec4 neighborColorSum{ 0.0f, 0.0f, 0.0f, 1.0f };
			for (int si = 0; si < 5; si++)
			{
				// 가로블러 픽셀데이터 받아와서
				Vec4 neighborColor = this->GetPixel(i, j + si - 2);
				neighborColorSum.v[0] += neighborColor.v[0] * weights[si];
				neighborColorSum.v[1] += neighborColor.v[1] * weights[si];
				neighborColorSum.v[2] += neighborColor.v[2] * weights[si];
			}
			// 가로 + 세로 블러로 적용
			pixelsBuffer[i + this->width * j].v[0] = neighborColorSum.v[0];
			pixelsBuffer[i + this->width * j].v[1] = neighborColorSum.v[1];
			pixelsBuffer[i + this->width * j].v[2] = neighborColorSum.v[2];

		}
	}

	// Swap
	std::swap(this->pixels, pixelsBuffer);
}

void Image::Bloom(const float& th, const int& numRepeat, const float& weight)
{
	//https://learnopengl.com/Advanced-Lighting/Bloom

	const std::vector<Vec4> pixelsBackup = this->pixels;// 메모리 내용물까지 모두 복사

	/* Brightness가 th 보다 작은 픽셀들을 모두 검은색으로 바꾸기
	* https://en.wikipedia.org/wiki/Relative_luminance
	* Relative Luminance Y = 0.2126*R + 0.7152*G + 0.0722*B
	*/
#pragma omp parallel for
	for (int j = 0; j < height; j++)
		for (int i = 0; i < width; i++)
		{
			auto& c = this->GetPixel(i, j);
			const float relativeLuminavce = c.v[0] * 0.2126f + c.v[1] * 0.7152f + c.v[2] * 0.0722f;
			if (relativeLuminavce < th)
			{
				c.v[0] = 0.f;
				c.v[1] = 0.f;
				c.v[2] = 0.f;
			}
		}

	// 여기서 Blur하지 않고 결과 확인

	// 밝은 부분만 Blur 
#pragma omp parallel for
	for (int i = 0; i < numRepeat; i++)
	{
		this->GaussianBlur5();
	}

	// 여기서 또 한 번 결과 확인

	// 밝은 부분만 Blur한 것과 원본 이미지를 더하기 (밝은 부분 Blur에 weight 곱해서 강도 조절)
	for (int i = 0; i < pixelsBackup.size(); i++)
	{
		this->pixels[i].v[0] = std::clamp(pixels[i].v[0] * weight + pixelsBackup[i].v[0], 0.0f, 1.0f);
		this->pixels[i].v[1] = std::clamp(pixels[i].v[1] * weight + pixelsBackup[i].v[1], 0.0f, 1.0f);
		this->pixels[i].v[2] = std::clamp(pixels[i].v[2] * weight + pixelsBackup[i].v[2], 0.0f, 1.0f);

	}
}
