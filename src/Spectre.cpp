#include "Spectre.h"
#include "string"
#include "vector"

#define GRAY						IM_COL32(95, 95, 95, 255)
#define SPECTRE_FREQ_MARK_COUNT		20
#define SPECTRE_DB_MARK_COUNT		10

//Upper right (spectreX1, spectreY1), down left (spectreX2, spectreY2)
bool Spectre::isMouseOnSpectreRegion(int spectreX1, int spectreY1, int spectreX2, int spectreY2) {
	ImGuiIO& io = ImGui::GetIO();
	if (io.MousePos.x >= spectreX1 && io.MousePos.x <= spectreX2 && io.MousePos.y >= spectreY1 && io.MousePos.y <= spectreY2) return true;
	return false;
}

Spectre::Spectre(Config* config, ViewModel* viewModel, FFTSpectreHandler* fftSH, double width, double height) {
	this->waterfall = new Waterfall(config, fftSH);
	//waterfall->start().detach();
	this->config = config;
	this->viewModel = viewModel;
	this->width = width;
	this->height = height;
	this->fftSH = fftSH;
	receiverLogicNew = new ReceiverLogicNew(config);
	maxdBKalman = new KalmanFilter(1, 0.005);
	ratioKalman = new KalmanFilter(10, 0.0001);
}

int savedStartWindowX = 0;
int savedEndWindowX = 0;
int savedSpectreWidthInPX = 1;

float veryMinSpectreVal = 0;
float veryMaxSpectreVal = -1000;

long countFrames = 0;


//#define checkImageWidth 128
//#define checkImageHeight 128



void Spectre::draw() {
	ImGui::Begin("Spectre");
		ImGuiIO& io = ImGui::GetIO();
		//ImDrawList* draw_list = ImGui::GetWindowDrawList();

		int rightPadding = 40;
		int leftPadding = 40;
		int waterfallPaddingTop = 50;

		//��������� ����� ����
		ImVec2 startWindowPoint = ImGui::GetCursorScreenPos();

		//������ ����� ����� ����
		ImVec2 windowLeftBottomCorner = ImGui::GetContentRegionAvail();
		int spectreHeight = windowLeftBottomCorner.y / 2;
		int waterfallHeight = windowLeftBottomCorner.y / 2;

		int spectreWidthInPX = windowLeftBottomCorner.x - rightPadding - leftPadding;

		//receiverLogicNew->setSpectrePositionX(startWindowPoint.x + rightPadding, startWindowPoint.x + windowLeftBottomCorner.x - leftPadding);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImGui::BeginChild("Spectre1", ImVec2(ImGui::GetContentRegionAvail().x, spectreHeight), false, ImGuiWindowFlags_NoMove);
			
			//Horizontal
			draw_list->AddLine(
				ImVec2(startWindowPoint.x + rightPadding, startWindowPoint.y + spectreHeight),
				ImVec2(startWindowPoint.x + windowLeftBottomCorner.x - leftPadding, startWindowPoint.y + spectreHeight),
				GRAY, 4.0f);

			//Vertical
			draw_list->AddLine(
				ImVec2(startWindowPoint.x + rightPadding, 0),
				ImVec2(startWindowPoint.x + rightPadding, startWindowPoint.y + spectreHeight),
				GRAY, 2.0f);

			//Freqs mark line
			int markCount = SPECTRE_FREQ_MARK_COUNT;
			int sampleRateStep = config->inputSamplerate / markCount;
			float stepInPX = spectreWidthInPX / markCount;
			//printf("%i\r\n", viewModel->frequency);
			//int step = viewModel->frequency - config->inputSamplerate / 2;
			for (int i = 0; i <= markCount; i++) {
				draw_list->AddText(
					ImVec2(startWindowPoint.x + rightPadding + i * stepInPX - 20.0, startWindowPoint.y + spectreHeight + 10.0), 
					IM_COL32_WHITE, 
					std::to_string((viewModel->frequency - config->inputSamplerate / 2) + i * sampleRateStep).c_str()
				);
					
				draw_list->AddLine(
					ImVec2(startWindowPoint.x + rightPadding + i * stepInPX, startWindowPoint.y + spectreHeight),
					ImVec2(startWindowPoint.x + rightPadding + i * stepInPX, startWindowPoint.y + spectreHeight - 5.0), 
					IM_COL32_WHITE, 2.0f
				);
			}
			//-----------------------------
	
			fftSH->getSemaphore()->lock();

			float* spectreData = fftSH->getOutput();

			storeSignaldB(spectreData);

			int spectreSize = fftSH->getSpectreSize();

			MIN_MAX m = getMinMaxInSpectre(spectreData, spectreSize);

			if (veryMinSpectreVal > m.min) veryMinSpectreVal = m.min;
			if (veryMaxSpectreVal < m.max) veryMaxSpectreVal = m.max;


			float stepX = (windowLeftBottomCorner.x - rightPadding - leftPadding)  / (spectreSize);

			float ratio = ratioKalman->filter(spectreHeight / (abs(veryMinSpectreVal) - abs(veryMaxSpectreVal)));

			float kaka = startWindowPoint.y - abs(veryMinSpectreVal) * ratio;
			float koeff = 0;
			if (abs(veryMinSpectreVal) * ratio > spectreHeight) {
				koeff = ( - 1) *  (abs(veryMinSpectreVal)* ratio - spectreHeight);
			} else {
				koeff = spectreHeight - abs(veryMinSpectreVal) * ratio;
			}
				
			for (int i = 0; i < spectreSize - 1; i++) {
				ImVec2 lineX1(startWindowPoint.x + (i * stepX) + rightPadding, startWindowPoint.y - spectreData[fftSH->getTrueBin(i)] * ratio + koeff);
				ImVec2 lineX2(startWindowPoint.x + ((i + 1) * stepX) + rightPadding, startWindowPoint.y - spectreData[fftSH->getTrueBin(i + 1)] * ratio + koeff);
				draw_list->AddLine(lineX1, lineX2, IM_COL32_WHITE, 2.0f);
			}

			//dB mark line

			stepInPX = spectreHeight / SPECTRE_DB_MARK_COUNT;
			float stepdB = (abs(veryMinSpectreVal) - abs(veryMaxSpectreVal)) / SPECTRE_DB_MARK_COUNT;

			for (int i = 0; i < SPECTRE_DB_MARK_COUNT; i++) {
				draw_list->AddText(
					ImVec2(startWindowPoint.x + rightPadding - 35, startWindowPoint.y + spectreHeight - i * stepInPX - 15),
					IM_COL32_WHITE, 
					std::to_string((int)round(veryMinSpectreVal + i * stepdB)).c_str()
				);
			}


			//printf("%f %f\r\n", m.min, m.max);

			//---------------

			fftSH->getSemaphore()->unlock();

			//����������� ���� ������� ����� ��������� ������ �������
			if (ImGui::IsMouseClicked(0) && isMouseOnSpectreRegion(startWindowPoint.x + rightPadding, startWindowPoint.y, startWindowPoint.x + windowLeftBottomCorner.x - leftPadding, startWindowPoint.y + spectreHeight)) {
				receiverLogicNew->saveDelta(io.MousePos.x - (startWindowPoint.x + rightPadding));
			}

			//����������� ���� ������� � ��������� ����� ��������� ������ �������
			if (ImGui::IsMouseDown(0) && isMouseOnSpectreRegion(startWindowPoint.x + rightPadding, startWindowPoint.y, startWindowPoint.x + windowLeftBottomCorner.x - leftPadding, startWindowPoint.y + spectreHeight)) {
				//ImGui::Text("Pipka: %g %g", io.MousePos.x, io.MousePos.y);
				receiverLogicNew->setPosition(io.MousePos.x - (startWindowPoint.x + rightPadding), spectreWidthInPX);
			}

			//���������� ���������� �� ���� �� x
			if (savedStartWindowX != startWindowPoint.x) {
				savedStartWindowX = startWindowPoint.x;
			}

			//���������� ��������� �� ������ ���� �� x
			if (savedEndWindowX != windowLeftBottomCorner.x) {
				receiverLogicNew->update(savedSpectreWidthInPX, spectreWidthInPX);
				savedSpectreWidthInPX = spectreWidthInPX;
				savedEndWindowX = windowLeftBottomCorner.x;
			}

			//receive region
			draw_list->AddLine(
				ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition(), 0),
				ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition(), startWindowPoint.y + spectreHeight),
				GRAY, 2.0f);


			float delta = receiverLogicNew->getFilterWidthAbs(viewModel->filterWidth);

			switch (viewModel->receiverMode) {
			case USB:

				draw_list->AddRectFilled(
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition(), startWindowPoint.y - 10),
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition() + delta, startWindowPoint.y + spectreHeight),
					IM_COL32(95, 95, 95, 125), 0);

				break;
			case LSB:

				draw_list->AddRectFilled(
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition() - delta, startWindowPoint.y - 10),
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition(), startWindowPoint.y + spectreHeight),
					IM_COL32(95, 95, 95, 125), 0);

				break;
			case AM:

				draw_list->AddRectFilled(
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition() - delta, startWindowPoint.y - 10),
					ImVec2(startWindowPoint.x + rightPadding + receiverLogicNew->getPosition() + delta, startWindowPoint.y + spectreHeight),
					IM_COL32(95, 95, 95, 125), 0);

				break;
			}
			//---

		ImGui::EndChild();

		ImGui::BeginChild("Waterfall", ImVec2(ImGui::GetContentRegionAvail().x, windowLeftBottomCorner.y - spectreHeight - 5), false, ImGuiWindowFlags_NoMove);

			//fftSH->getSemaphore()->lock();

			//float* spectreDataCopy = new float[spectreSize];
			//memset(spectreDataCopy, 0, sizeof(float) * spectreSize);

			//memcpy(spectreDataCopy, spectreData, sizeof(spectreData)* spectreSize);
			//fftSH->getSemaphore()->unlock();

			//spectreSize = fftSH->getSpectreSize();
			if (countFrames % 1 == 0) {
				waterfall->setMinMaxValue(veryMinSpectreVal, veryMaxSpectreVal);
				waterfall->putData(fftSH, spectreData, spectreSize);
			}

			stepX = spectreWidthInPX / (spectreSize / waterfall->getDiv());
			float stepY = (windowLeftBottomCorner.y / 2) / waterfall->getSize();


			for (int i = 0; i < waterfall->getSize(); i++) {
				draw_list->AddImage((void*)(intptr_t)waterfall->getTexturesArray()[i], ImVec2(startWindowPoint.x + rightPadding, startWindowPoint.y + 30 + spectreHeight + stepY * i), ImVec2(startWindowPoint.x + spectreWidthInPX, startWindowPoint.y + 30 + spectreHeight + stepY * i + stepX));
				//draw_list->AddImage((void*)(intptr_t)waterfall->getTexturesArray()[i], ImVec2(startWindowPoint.x, startWindowPoint.y + spectreHeight), ImVec2(startWindowPoint.x + spectreWidthInPX, startWindowPoint.y + spectreHeight));
			}


			//Waterfall::WATERFALL_TEXTURE_STRUCT textStruct = waterfall->getTextureStruct();

			//Waterfall::WATERFALL_TEXTURE_STRUCT textStruct = waterfall->generateWaterfallTexture();

			//stepX = spectreWidthInPX / (spectreSize / waterfall->getDiv());

			//ImGui::Text("pointer = %p", textStruct.texName);
			//ImGui::Image((void*)(intptr_t)textStruct.texName, ImVec2(stepX * textStruct.width, textStruct.height));
			//draw_list->AddImage((void*)(intptr_t)textStruct.texName, ImVec2(startWindowPoint.x, startWindowPoint.y + spectreHeight + 50), ImVec2(startWindowPoint.x + spectreWidthInPX, startWindowPoint.y + textStruct.height + 200));
			
/*
			//delete spectreDataCopy;


			//Utils::printFloat(waterfallData.size());

			for (int j = 0; j < waterfall.getSize(); j++) {
				for (int i = 0; i < (spectreSize / waterfall.getDiv()); i++) {
					float* currentSpectre = waterfall.getDataFor((waterfall.getSize() - 1) - j);


					Waterfall::RGB rgb = waterfall.getColorForPowerInSpectre(currentSpectre[i]);

					ImVec2 lineX1(
						startWindowPoint.x + i * stepX + rightPadding, 
						startWindowPoint.y + spectreHeight + waterfallPaddingTop + stepX * j
					);
					ImVec2 lineX2(
						startWindowPoint.x + (i + 1.0) * stepX + rightPadding, 
						startWindowPoint.y + spectreHeight + waterfallPaddingTop + stepX * (j + 1.0)
					);
					draw_list->AddRectFilled(lineX1, lineX2, IM_COL32(rgb.r, rgb.g, rgb.b, 255), 0.0f);
				}
			}*/

		ImGui::EndChild();

	ImGui::End();
	countFrames++;



	/*for (int ix = 0; ix < checkImageHeight; ++ix) {
		for (int iy = 0; iy < checkImageWidth; ++iy) {
			int c = (((ix & 0x8) == 0) ^ ((iy & 0x8)) == 0) * 255;

			checkImage[ix * checkImageWidth * depth + iy * depth + 0] = c;   //red
			checkImage[ix * checkImageWidth * depth + iy * depth + 1] = c;   //green
			checkImage[ix * checkImageWidth * depth + iy * depth + 2] = c;   //blue
			checkImage[ix * checkImageWidth * depth + iy * depth + 3] = 255; //alpha
		}
	}*/	



}

void Spectre::storeSignaldB(float* spectreData) {
	ReceiverLogicNew::ReceiveBinArea r = receiverLogicNew->getReceiveBinsArea(viewModel->filterWidth, viewModel->receiverMode);

	//viewModel->serviceField1 = r.A;

	float sum = 0.0;
	int len = r.B - r.A;

	if (len > 0) {
		//Utils::printArray(spectre, 64);
		//printf("%i %i\r\n", r.A, r.B);

		float max = -1000.0;

		for (int i = r.A; i < r.B; i++) {
			if (spectreData[fftSH->getTrueBin(i)] > max) {
				max = spectreData[fftSH->getTrueBin(i)];
			}
			//sum += spectre[i];
		}
		viewModel->signalMaxdB = maxdBKalman->filter(max);
	}
}

Spectre::MIN_MAX Spectre::getMinMaxInSpectre(float* spectreData, int len) {
	float min = 1000.0;
	float max = -1000.0;
	for (int i = 0; i < len; i++) {
		if (spectreData[i] < min) min = spectreData[i];
		if (spectreData[i] > max) max = spectreData[i];
	}
	return MIN_MAX { min, max };
}