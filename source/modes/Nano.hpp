#pragma once
#include "../Utils.hpp"
#include "../include/t210.h"

class MainMenu;
class ConfiguratorOverlay;

class NanoOverlayElement : public tsl::elm::Element {
public:
    NanoOverlayElement(tsl::elm::Element* status, tsl::elm::Element* fpsGraph)
        : m_status(status), m_fpsGraph(fpsGraph) {}
    virtual void draw(tsl::gfx::Renderer* renderer) override {
        m_status->draw(renderer);
        m_fpsGraph->draw(renderer);
    }
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        m_status->setBoundaries(parentX, parentY, parentWidth, parentHeight);
        m_status->invalidate();
        m_fpsGraph->setBoundaries(parentX, parentY, parentWidth, parentHeight);
        m_fpsGraph->invalidate();
    }
private:
    tsl::elm::Element* m_status;
    tsl::elm::Element* m_fpsGraph;
};

class NanoOverlay : public tsl::Gui {
	
struct stats {
    s16 value;
    bool zero_rounded;
};

private:
    std::vector<double> cpu_usage_history;
    std::vector<double> gpu_load_history;
    FpsGraphSettings fpsGraphSettings;
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_var_compressed_c[128] = "";
    char SoCPCB_temperature_c[64] = "";
    char skin_temperature_c[32] = "";
    char Variables[512] = "";
    char VariablesB[512] = "";
    char CPU_compressed_c[160] = "";
    char CPU_Usage0[32] = "";
    char CPU_Usage1[32] = "";
    char CPU_Usage2[32] = "";
    char CPU_Usage3[32] = "";
    char RAM_all_c[64] = "";
    char RAM_freq_c[8] = "";
    char FPSavg_c[8] = "";
    char FPS_var_compressed_c[64] = "";
    uint32_t rectangleWidth = 0;
    size_t fontsize = 0;
    NanoSettings settings;
    bool Initialized = false;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    const size_t history_size = 30;
    
    std::vector<stats> readings;
    s16 base_y = 0;
    s16 base_x = 0;
    s16 rectangle_width = 180 + 52;  // Doubled width to fit more values
    s16 rectangle_height = 72;
    s16 rectangle_x = 5;
    s16 rectangle_y = 5;
    s16 rectangle_range_max = 72;
    s16 rectangle_range_min = 0;
    char legend_max[3] = "72";
    char legend_min[2] = "0";
    s32 range = std::abs(rectangle_range_max - rectangle_range_min) + 1;
    s16 x_end = rectangle_x + rectangle_width;
    s16 y_old = rectangle_y+rectangle_height;
    s16 y_30FPS = rectangle_y+(rectangle_height / 2);
    s16 y_60FPS = rectangle_y;
    bool isAbove = false;
    Mutex fpsGraphMutex;
    char LCD_Hz_c[8];
    std::vector<uint32_t> lcd_hz_samples;
    uint32_t lcd_hz_sum;
    uint32_t lcd_hz_avg;
    uint64_t last_lcd_hz_update;
    uint64_t last_indicator_update = 0;
    tsl::elm::HeaderOverlayFrame* rootFrame = nullptr;
    tsl::elm::CustomDrawer* statusDrawer = nullptr;
    tsl::elm::CustomDrawer* fpsGraphDrawer = nullptr;
    
    // Store last settings values to detect changes
    size_t lastHandheldFontSize = 0;
    size_t lastDockedFontSize = 0;
    uint16_t lastTextColor = 0;
    bool lastUseGradient = false;
    uint32_t lastGradientStartColor = 0;
    uint32_t lastGradientEndColor = 0;
		
	void drawGradientText(tsl::gfx::Renderer *renderer, const char* text, u32 x, u32 y, u16 w, u16 h) {
		size_t textLength = strlen(text);
		
		if (settings.useGradient && textLength > 1) {
			// Use gradient colors (RGB888 format)
			u32 startColor = settings.gradientStartColor;
			u32 endColor = settings.gradientEndColor;
			
			u8 startR = (startColor >> 16) & 0xFF;
			u8 startG = (startColor >> 8) & 0xFF;
			u8 startB = startColor & 0xFF;
			
			u8 endR = (endColor >> 16) & 0xFF;
			u8 endG = (endColor >> 8) & 0xFF;
			u8 endB = endColor & 0xFF;
			
			for (size_t i = 0; i < textLength; i++) {
				float progress = textLength > 1 ? static_cast<float>(i) / (textLength - 1) : 0.0f;
				
				u8 r = static_cast<u8>((1 - progress) * startR + progress * endR);
				u8 g = static_cast<u8>((1 - progress) * startG + progress * endG);
				u8 b = static_cast<u8>((1 - progress) * startB + progress * endB);
				
				tsl::Color color(static_cast<u8>(r >> 4), static_cast<u8>(g >> 4), static_cast<u8>(b >> 4), 0xF);
				
				char buffer[2] = {text[i], '\0'};
				renderer->drawString(buffer, false, x, y, fontsize, color);
				
				auto [width, height] = renderer->drawString(buffer, false, 0, 0, fontsize, tsl::style::color::ColorTransparent);
				x += width;
			}
		} else {
			// Use solid color
			u16 textColorValue = settings.textColor;
			u8 r = ((textColorValue >> 12) & 0xF) << 4;
			u8 g = ((textColorValue >> 8) & 0xF) << 4;
			u8 b = ((textColorValue >> 4) & 0xF) << 4;
			u8 a = (textColorValue & 0xF) << 4;
			
			for (size_t i = 0; i < textLength; i++) {
				tsl::Color color(static_cast<u8>(r >> 4), static_cast<u8>(g >> 4), static_cast<u8>(b >> 4), static_cast<u8>(a >> 4));
				
				char buffer[2] = {text[i], '\0'};
				renderer->drawString(buffer, false, x, y, fontsize, color);
				
				auto [width, height] = renderer->drawString(buffer, false, 0, 0, fontsize, tsl::style::color::ColorTransparent);
				x += width;
			}
		}
	}		
	void drawGradientTextB(tsl::gfx::Renderer *renderer, const char* text, u32 x, u32 y, u16 w, u16 h) {
		size_t textLength = strlen(text);
		
		if (settings.useGradient && textLength > 1) {
			// Use gradient colors (RGB888 format)
			u32 startColor = settings.gradientStartColor;
			u32 endColor = settings.gradientEndColor;
			
			u8 startR = (startColor >> 16) & 0xFF;
			u8 startG = (startColor >> 8) & 0xFF;
			u8 startB = startColor & 0xFF;
			
			u8 endR = (endColor >> 16) & 0xFF;
			u8 endG = (endColor >> 8) & 0xFF;
			u8 endB = endColor & 0xFF;
			
			for (size_t i = 0; i < textLength; i++) {
				float progress = textLength > 1 ? static_cast<float>(i) / (textLength - 1) : 0.0f;
				
				u8 r = static_cast<u8>((1 - progress) * startR + progress * endR);
				u8 g = static_cast<u8>((1 - progress) * startG + progress * endG);
				u8 b = static_cast<u8>((1 - progress) * startB + progress * endB);
				
				tsl::Color color(static_cast<u8>(r >> 4), static_cast<u8>(g >> 4), static_cast<u8>(b >> 4), 0xF);
				
				char buffer[2] = {text[i], '\0'};
				renderer->drawString(buffer, false, x, y, fontsize, color);
				
				auto [width, height] = renderer->drawString(buffer, false, 0, 0, fontsize, tsl::style::color::ColorTransparent);
				x += width;
			}
		} else {
			// Use solid color
			u16 textColorValue = settings.textColor;
			u8 r = ((textColorValue >> 12) & 0xF) << 4;
			u8 g = ((textColorValue >> 8) & 0xF) << 4;
			u8 b = ((textColorValue >> 4) & 0xF) << 4;
			u8 a = (textColorValue & 0xF) << 4;
			
			for (size_t i = 0; i < textLength; i++) {
				tsl::Color color(static_cast<u8>(r >> 4), static_cast<u8>(g >> 4), static_cast<u8>(b >> 4), static_cast<u8>(a >> 4));
				
				char buffer[2] = {text[i], '\0'};
				renderer->drawString(buffer, false, x, y, fontsize, color);
				
				auto [width, height] = renderer->drawString(buffer, false, 0, 0, fontsize, tsl::style::color::ColorTransparent);
				x += width;
			}
		}
	}
	
// Упрощенная функция для рисования линии одного цвета
void drawSimpleLine(tsl::gfx::Renderer *renderer, s16 x1, s16 y1, s16 x2, s16 y2, u32 color) {
    tsl::Color lineColor( 
        static_cast<u8>(((color >> 16) & 0xFF) >> 4), 
        static_cast<u8>(((color >> 8) & 0xFF) >> 4), 
        static_cast<u8>((color & 0xFF) >> 4), 
        0xF 
    );
    renderer->drawLine(x1, y1, x2, y2, lineColor);
}
	
public:
	NanoOverlay() : lcd_hz_sum(0), lcd_hz_avg(60), last_lcd_hz_update(0) {
		GetConfigSettings(&settings);
		GetConfigSettings(&fpsGraphSettings);
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Normal) {
			fontsize = settings.handheldFontSize;
		}
		else fontsize = settings.dockedFontSize;
		
		// Store initial settings for comparison
		lastHandheldFontSize = settings.handheldFontSize;
		lastDockedFontSize = settings.dockedFontSize;
		lastTextColor = settings.textColor;
		lastUseGradient = settings.useGradient;
		lastGradientStartColor = settings.gradientStartColor;
		lastGradientEndColor = settings.gradientEndColor;
		
		switch(settings.setPos) {
			case 1:
			case 4:
			case 7:
				tsl::gfx::Renderer::getRenderer().setLayerPos(624, 0);
				break;
			case 2:
			case 5:
			case 8:
				tsl::gfx::Renderer::getRenderer().setLayerPos(1248, 0);
				break;
		}
		cpu_usage_history = std::vector<double>(history_size, 0.0);
		gpu_load_history = std::vector<double>(history_size, 0.0);
		mutexInit(&mutex_BatteryChecker);
		mutexInit(&mutex_Misc);
		tsl::hlp::requestForeground(false);
		FullMode = false;
		
		if (settings.disableScreenshots) {
			tsl::gfx::Renderer::get().removeScreenshotStacks();
		}
		
		TeslaFPS = 60;
		systemtickfrequency_impl = systemtickfrequency / TeslaFPS;
		
		deactivateOriginalFooter = true;
		StartThreads();
		mutexInit(&fpsGraphMutex);
		snprintf(LCD_Hz_c, sizeof(LCD_Hz_c), "LCD:--");
		snprintf(FPSavg_c, sizeof(FPSavg_c), "--");
	}
	
    ~NanoOverlay() {
		CloseThreads();
		FullMode = true;
		tsl::hlp::requestForeground(true);
		if (settings.disableScreenshots) {
			tsl::gfx::Renderer::get().addScreenshotStacks();
		}
		deactivateOriginalFooter = false;
    }
    virtual tsl::elm::Element* createUI() override {
        rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        statusDrawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            drawGradientText(renderer, Variables, 4, 14, w, h);
            drawGradientTextB(renderer, VariablesB, 4, 30, w, h);
        });
	fpsGraphDrawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
		switch(settings.setPos) {
			case 1:
				base_x = 224 - ((rectangle_width + 21) / 2);
				break;
			case 4:
				base_x = 224 - ((rectangle_width + 21) / 2);
				base_y = 360 - ((rectangle_height + 12 + 24) / 2);
				break;
			case 7:
				base_x = 224 - ((rectangle_width + 21) / 2);
				base_y = 720 - (rectangle_height + 12 + 24);
				break;
			case 2:
				base_x = 448 - (rectangle_width + 21);
				break;
			case 5:
				base_x = 448 - (rectangle_width + 21);
				base_y = 360 - ((rectangle_height + 12 + 24) / 2);
				break;
			case 8:
				base_x = 448 - (rectangle_width + 21);
				base_y = 720 - (rectangle_height + 12 + 24);
				break;
		}

		// Calculate graph colors based on gradient mode
		u32 lineColor1, lineColor2, lineColor3;
		if (settings.useGradient) {
			// Use gradient colors (RGB888 format)
			lineColor1 = 0xFF000000 | settings.gradientStartColor; // Top line - start color
			// Middle line - interpolated color (50% between start and end)
			u8 midR = ((settings.gradientStartColor >> 16) & 0xFF) / 2 + ((settings.gradientEndColor >> 16) & 0xFF) / 2;
			u8 midG = ((settings.gradientStartColor >> 8) & 0xFF) / 2 + ((settings.gradientEndColor >> 8) & 0xFF) / 2;
			u8 midB = (settings.gradientStartColor & 0xFF) / 2 + (settings.gradientEndColor & 0xFF) / 2;
			lineColor2 = 0xFF000000 | (midR << 16) | (midG << 8) | midB;
			lineColor3 = 0xFF000000 | settings.gradientEndColor; // Bottom line - end color
		} else {
			// Use text color (RGBA4444 format) - convert to RGB888
			u8 r = ((settings.textColor >> 12) & 0xF) << 4;
			u8 g = ((settings.textColor >> 8) & 0xF) << 4;
			u8 b = ((settings.textColor >> 4) & 0xF) << 4;
			u32 textColorRGB = (r << 16) | (g << 8) | b;
			
			// Create slight variations for visual effect
			lineColor1 = 0xFF000000 | textColorRGB; // Top line - full brightness
			// Middle line - slightly dimmed
			u8 midR = (r * 9) / 10;
			u8 midG = (g * 9) / 10;
			u8 midB = (b * 9) / 10;
			lineColor2 = 0xFF000000 | (midR << 16) | (midG << 8) | midB;
			// Bottom line - more dimmed
			u8 dimR = (r * 8) / 10;
			u8 dimG = (g * 8) / 10;
			u8 dimB = (b * 8) / 10;
			lineColor3 = 0xFF000000 | (dimR << 16) | (dimG << 8) | dimB;
		}
		
		// Lock mutex before accessing readings to prevent race conditions
		mutexLock(&fpsGraphMutex);
		size_t readings_size = readings.size();
		size_t last_element = readings_size > 0 ? readings_size - 1 : 0;
		
		// Create a copy of readings to avoid holding mutex during rendering
		std::vector<stats> readings_copy;
		if (readings_size > 0) {
			readings_copy = readings;
		}
		mutexUnlock(&fpsGraphMutex);
		
		// Draw graph using the copy
		for (s16 x = x_end; x > static_cast<s16>(x_end-readings_copy.size()); x--) {
			if (last_element >= readings_copy.size()) break;
			s32 y_on_range = readings_copy[last_element].value + std::abs(rectangle_range_min) + 1;
			if (y_on_range < 0) {
				y_on_range = 0;
			}
			else if (y_on_range > range) {
				isAbove = true;
				y_on_range = range;
			}

			s16 y = rectangle_y + static_cast<s16>(std::lround((float)rectangle_height * ((float)(range - y_on_range) / (float)range)));
			drawSimpleLine(renderer, base_x + x, base_y + y + 29, base_x + x, base_y + y_old + 29, lineColor1);
			drawSimpleLine(renderer, base_x + x, base_y + y + 31, base_x + x, base_y + y_old + 31, lineColor2);
			drawSimpleLine(renderer, base_x + x, base_y + y + 33, base_x + x, base_y + y_old + 33, lineColor3);
			isAbove = false;
			y_old = y;
			if (last_element > 0) {
				last_element--;
			} else {
				break;
			}
		}
	});

        auto overlayElement = new NanoOverlayElement(statusDrawer, fpsGraphDrawer);
        rootFrame->setContent(overlayElement);
        return rootFrame;
    }
	
	virtual void update() override {
		// Reload settings if they changed
		GetConfigSettings(&settings);
		
		// Check if settings changed and need UI update
		bool settingsChanged = false;
		if (lastHandheldFontSize != settings.handheldFontSize ||
		    lastDockedFontSize != settings.dockedFontSize ||
		    lastTextColor != settings.textColor ||
		    lastUseGradient != settings.useGradient ||
		    lastGradientStartColor != settings.gradientStartColor ||
		    lastGradientEndColor != settings.gradientEndColor) {
			settingsChanged = true;
			lastHandheldFontSize = settings.handheldFontSize;
			lastDockedFontSize = settings.dockedFontSize;
			lastTextColor = settings.textColor;
			lastUseGradient = settings.useGradient;
			lastGradientStartColor = settings.gradientStartColor;
			lastGradientEndColor = settings.gradientEndColor;
		}
		
		// Update fontsize based on performance mode
		apmGetPerformanceMode(&performanceMode);
		if (performanceMode == ApmPerformanceMode_Normal) {
			if (fontsize != settings.handheldFontSize) {
				fontsize = settings.handheldFontSize;
				settingsChanged = true;
			}
		}
		else {
			if (fontsize != settings.dockedFontSize) {
				fontsize = settings.dockedFontSize;
				settingsChanged = true;
			}
		}
		
		// Invalidate UI if settings changed
		if (settingsChanged) {
			if (statusDrawer) {
				statusDrawer->invalidate();
			}
			if (fpsGraphDrawer) {
				fpsGraphDrawer->invalidate();
			}
		}
		
		uint32_t current_lcd_hz = GetHzLCD();
		uint64_t current_time = svcGetSystemTick();

		lcd_hz_samples.push_back(current_lcd_hz);
		lcd_hz_sum += current_lcd_hz;

		if (current_time - last_lcd_hz_update >= 19200000) {
			lcd_hz_avg = lcd_hz_sum / lcd_hz_samples.size();
			lcd_hz_sum = 0;
			lcd_hz_samples.clear();
			last_lcd_hz_update = current_time;
			
			if (lcd_hz_avg > 0) {
				TeslaFPS = lcd_hz_avg;
				systemtickfrequency_impl = systemtickfrequency / TeslaFPS;
			} else {
				TeslaFPS = 60; // Default 60 Hz if LCD frequency cannot be determined
				systemtickfrequency_impl = systemtickfrequency / TeslaFPS;
			}
		}

		// Control update rate for indicators using settings.updateRate
		uint64_t updateInterval = systemtickfrequency / settings.updateRate;
		bool shouldUpdateIndicators = (current_time - last_indicator_update >= updateInterval);
		
		if (shouldUpdateIndicators) {
			last_indicator_update = current_time;

			if (idletick0 > systemtickfrequency_impl)
				strcpy(CPU_Usage0, "0%");
			if (idletick1 > systemtickfrequency_impl)
				strcpy(CPU_Usage1, "0%");
			if (idletick2 > systemtickfrequency_impl)
				strcpy(CPU_Usage2, "0%");
			if (idletick3 > systemtickfrequency_impl)
				strcpy(CPU_Usage3, "0%");

			double percent0 = (1.0 - ((double)idletick0.load(std::memory_order_acquire) / systemtickfrequency_impl)) * 100;
			double percent1 = (1.0 - ((double)idletick1.load(std::memory_order_acquire) / systemtickfrequency_impl)) * 100;
			double percent2 = (1.0 - ((double)idletick2.load(std::memory_order_acquire) / systemtickfrequency_impl)) * 100;
			double percent3 = (1.0 - ((double)idletick3.load(std::memory_order_acquire) / systemtickfrequency_impl)) * 100;
			double average_percent = (percent0 + percent1 + percent2 + percent3) / 4.0;

			average_percent = std::max(0.0, average_percent);

			cpu_usage_history.push_back(average_percent);
			if (cpu_usage_history.size() > history_size) {
				cpu_usage_history.erase(cpu_usage_history.begin());
			}

			float smoothed_average = 0;
			for (const auto& value : cpu_usage_history) {
				smoothed_average += value;
			}
			smoothed_average /= cpu_usage_history.size();

			mutexLock(&mutex_Misc);
			snprintf(CPU_compressed_c, sizeof CPU_compressed_c, "%2.0f%%%5.0f", smoothed_average, (float)CPU_Hz / 1000000);

			float gpu_load = (double)GPU_Load_u / 10.0;

			gpu_load_history.push_back(gpu_load);
			if (gpu_load_history.size() > history_size) {
				gpu_load_history.erase(gpu_load_history.begin());
			}

			float smoothed_gpu_load = 0;
			for (const auto& value : gpu_load_history) {
				smoothed_gpu_load += value;
			}
			smoothed_gpu_load /= gpu_load_history.size();

			snprintf(GPU_Load_c, sizeof GPU_Load_c, "%.0f%%%4.0f", smoothed_gpu_load, (float)GPU_Hz / 1000000);

			float RAM_Total_application_f = (float)RAM_Total_application_u / 1024 / 1024;
			float RAM_Total_applet_f = (float)RAM_Total_applet_u / 1024 / 1024;
			float RAM_Total_system_f = (float)RAM_Total_system_u / 1024 / 1024;
			float RAM_Total_systemunsafe_f = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
			float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
			float RAM_Used_application_f = (float)RAM_Used_application_u / 1024 / 1024;
			float RAM_Used_applet_f = (float)RAM_Used_applet_u / 1024 / 1024;
			float RAM_Used_system_f = (float)RAM_Used_system_u / 1024 / 1024;
			float RAM_Used_systemunsafe_f = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
			float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
			snprintf(RAM_all_c, sizeof RAM_all_c, "%.0f/%.0fMB", RAM_Used_all_f, RAM_Total_all_f);
			snprintf(RAM_var_compressed_c, sizeof RAM_var_compressed_c, "%s@%.1f", RAM_all_c, (float)RAM_Hz / 1000000);
		
			if (settings.realFrequencies && realRAM_Hz) {
				snprintf(RAM_freq_c, sizeof RAM_freq_c, "%d", realRAM_Hz / 1000000);
			} else {
				snprintf(RAM_freq_c, sizeof RAM_freq_c, "%d", RAM_Hz / 1000000);
			}

			if (performanceMode == ApmPerformanceMode_Normal) {
				if (FPS > lcd_hz_avg) FPS = lcd_hz_avg;
				snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "%u/%u HZ", FPS, lcd_hz_avg);
			} else {
				snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "%u", FPS);
			}

			double fps_per_watt = FPSavg / (PowerConsumption * -1);
			char fpsPerWatt_c[64];
			if (PowerConsumption > -1) {
				snprintf(fpsPerWatt_c, sizeof(fpsPerWatt_c), " ");
			} else {
				int num_stars = (int)round(fps_per_watt / 4);
				snprintf(fpsPerWatt_c, sizeof(fpsPerWatt_c), " FpW %2.0f ", fps_per_watt);
				for (int i = 0; i < num_stars; ++i) {
					strncat(fpsPerWatt_c, "★", sizeof(fpsPerWatt_c) - strlen(fpsPerWatt_c) - 1); 
				}
			}
			PowerConsumption *= 1.0;
			snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c, "%0.2fW", PowerConsumption);
			if (hosversionAtLeast(14,0,0))
				snprintf(skin_temperature_c, sizeof skin_temperature_c, "%2.0f\u00B0C", (float)skin_temperaturemiliC / 1000);
			else
				snprintf(skin_temperature_c, sizeof skin_temperature_c, "%2.0f\u00B0C", (float)skin_temperaturemiliC / 1000);
			snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%2.0f%%", Rotation_Duty);

			bool showPower = (PowerConsumption < -0.05 || PowerConsumption > 0.05);
			
			// Format battery time estimate
			char remainingBatteryLife[8] = "--:--";
			if (settings.showBatteryTime) {
				mutexLock(&mutex_BatteryChecker);
				const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f : PowerConsumption;
				if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
					snprintf(remainingBatteryLife, sizeof(remainingBatteryLife),
							 "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
				}
				mutexUnlock(&mutex_BatteryChecker);
			}

			if (GameRunning) {
				snprintf(Variables, sizeof Variables, "GPU %s  CPU%s  MEM %s",
						 GPU_Load_c, CPU_compressed_c, RAM_freq_c);

				if (showPower) {
					if (settings.showBatteryTime) {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %s %+.1fW [%s]",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, fpsPerWatt_c, PowerConsumption, remainingBatteryLife);
					} else {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %s %+.1fW",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, fpsPerWatt_c, PowerConsumption);
					}
				} else {
					if (settings.showBatteryTime) {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %s [%s]",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, fpsPerWatt_c, remainingBatteryLife);
					} else {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %s",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, fpsPerWatt_c);
					}
				}
			} else {
				snprintf(Variables, sizeof Variables, "GPU %s  CPU%s  MEM %s",
						 GPU_Load_c, CPU_compressed_c, RAM_freq_c);

				if (showPower) {
					if (settings.showBatteryTime) {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %+.1fW [%s]",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, PowerConsumption, remainingBatteryLife);
					} else {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s %+.1fW",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, PowerConsumption);
					}
				} else {
					if (settings.showBatteryTime) {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s [%s]",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c, remainingBatteryLife);
					} else {
						snprintf(VariablesB, sizeof VariablesB, "FPS %s  %s%s",
								 FPS_var_compressed_c, skin_temperature_c, Rotation_SpeedLevel_c);
					}
				}
			}
			mutexUnlock(&mutex_Misc);
		}
		
		// Update FPS graph (like FPS Graph mode - in update() method, not separate thread)
		static uint64_t lastFrame = 0;
		stats temp = {0, false};
		
		snprintf(FPSavg_c, sizeof FPSavg_c, "%2.1f",  FPSavg);
		if (FPSavg < 254) {
			// Only update when new frame data is available (like FPS Graph mode)
			if (lastFrame == lastFrameNumber) {
				// No new frames, skip update
			} else {
				lastFrame = lastFrameNumber;
				mutexLock(&fpsGraphMutex);
				if ((s16)(readings.size()) >= rectangle_width) {
					readings.erase(readings.begin());
				}
				float whole = std::round(FPSavg);
				temp.value = static_cast<s16>(std::lround(FPSavg));
				if (FPSavg < whole+0.04 && FPSavg > whole-0.05) {
					temp.zero_rounded = true;
				}
				readings.push_back(temp);
				mutexUnlock(&fpsGraphMutex);
			}
		}
		else {
			// Game closed - FPSavg is 254, clear graph
			if (readings.size()) {
				mutexLock(&fpsGraphMutex);
				readings.clear();
				readings.shrink_to_fit();
				mutexUnlock(&fpsGraphMutex);
				lastFrame = 0;
			}
			FPSavg_c[0] = 0;
		}
	}
	
	virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
		if (isKeyComboPressed(keysHeld, keysDown)) {
			TeslaFPS = 60;
			tsl::goBack();
			return true;
		}
		return false;
	}
};

