#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp> 
#include "auto_tracker.h"
#include <iostream>
#include <vector>

using namespace cv;

const int PROCESSING_WIDTH = 800; 

cv::Mat GetProcessedFrame(Texture2D texture, float& outScale) {
    Image img = LoadImageFromTexture(texture);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    
    cv::Mat original(img.height, img.width, CV_8UC3, img.data);
    
    cv::GaussianBlur(original, original, cv::Size(3, 3), 0);
    
    float scale = 1.0f;
    cv::Mat resized;
    
    if (original.cols > PROCESSING_WIDTH) {
        scale = (float)PROCESSING_WIDTH / original.cols;
        cv::resize(original, resized, cv::Size(), scale, scale, INTER_LINEAR);
    } else {
        resized = original.clone();
    }
    
    outScale = scale;
    UnloadImage(img);

    cv::cvtColor(resized, resized, cv::COLOR_RGB2BGR);

    cv::Mat lab;
    cv::cvtColor(resized, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab, lab_planes); 

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.5, cv::Size(8, 8));
    clahe->apply(lab_planes[0], lab_planes[0]);

    cv::merge(lab_planes, lab);
    cv::cvtColor(lab, resized, cv::COLOR_Lab2BGR);

    return resized;
}

struct TrackerWrapper {
    cv::Ptr<cv::Tracker> cvTracker;
    int lostFrameCount; 
};

extern "C" {

void AutoTracker_Init(AutoTracker *tracker) {
    tracker->state = TRACKER_IDLE;
    tracker->needsToAdvance = false;
    tracker->targetRect = Rectangle{0};
    tracker->velocity = Vector2{0,0};
    tracker->searchWindowScale = 1.0f;
    tracker->colorTolerance = 0.15f; 
    tracker->targetColor = WHITE;
    
    tracker->internal_ptr = new TrackerWrapper();
    ((TrackerWrapper*)tracker->internal_ptr)->lostFrameCount = 0;
}

void AutoTracker_StartSelection(AutoTracker *tracker) {
    tracker->state = TRACKER_SELECTING;
}

void AutoTracker_ConfirmSelection(AutoTracker *tracker, VideoEngine *video) {
    if (!video->isLoaded) return;
    TrackerWrapper* wrapper = (TrackerWrapper*)tracker->internal_ptr;
    wrapper->lostFrameCount = 0;
    
    try {
        wrapper->cvTracker = cv::TrackerCSRT::create();
        
        float scale = 1.0f;
        cv::Mat frame = GetProcessedFrame(video->texture, scale);

        float padding = 4.0f; 
        float targetW = tracker->targetRect.width;
        float targetH = tracker->targetRect.height;
        
        if (targetW < 10) { targetW = 20; tracker->targetRect.x -= 5; }
        if (targetH < 10) { targetH = 20; tracker->targetRect.y -= 5; }

        int scaledX = (int)((tracker->targetRect.x - padding) * scale);
        int scaledY = (int)((tracker->targetRect.y - padding) * scale);
        int scaledW = (int)((targetW + padding*2) * scale);
        int scaledH = (int)((targetH + padding*2) * scale);

        if (scaledX < 0) scaledX = 0;
        if (scaledY < 0) scaledY = 0;
        if (scaledX + scaledW >= frame.cols) scaledW = frame.cols - scaledX - 1;
        if (scaledY + scaledH >= frame.rows) scaledH = frame.rows - scaledY - 1;

        if (scaledW <= 0 || scaledH <= 0) {
            printf("[OpenCV] Selection invalide (trop pres du bord). Annulation.\n");
            tracker->state = TRACKER_IDLE;
            return;
        }

        cv::Rect bbox(scaledX, scaledY, scaledW, scaledH);
        wrapper->cvTracker->init(frame, bbox);
        
        tracker->state = TRACKER_READY;
        tracker->centerPos = Vector2{ 
            tracker->targetRect.x + tracker->targetRect.width/2.0f, 
            tracker->targetRect.y + tracker->targetRect.height/2.0f 
        };
        tracker->velocity = Vector2{0,0};
        printf("[OpenCV] Tracker Ready.\n");

    } catch (const cv::Exception& e) {
        printf("[OpenCV CRASH EVITE] Init failed: %s\n", e.what());
        tracker->state = TRACKER_IDLE;
    }
}

void AutoTracker_StartTracking(AutoTracker *tracker) {
    if (tracker->state == TRACKER_READY || tracker->state == TRACKER_LOST) {
        tracker->state = TRACKER_TRACKING;
        tracker->needsToAdvance = false;
    }
}

void AutoTracker_Stop(AutoTracker *tracker) {
    tracker->state = TRACKER_IDLE;
    tracker->needsToAdvance = false;
}

void AutoTracker_Cancel(AutoTracker *tracker) {
    tracker->state = TRACKER_IDLE;
}

void AutoTracker_Update(AutoTracker *tracker, VideoEngine *video, TrackingSystem *ts) {
    if (tracker->state != TRACKER_TRACKING || !video->isLoaded) return;

    TrackerWrapper* wrapper = (TrackerWrapper*)tracker->internal_ptr;
    if (!wrapper || wrapper->cvTracker.empty()) return;

    float scale = 1.0f;
    
    try {
        cv::Mat frame = GetProcessedFrame(video->texture, scale);
        cv::Rect bbox;
        bool ok = wrapper->cvTracker->update(frame, bbox);
        
        if (!ok && wrapper->lostFrameCount < 3) {
            float predX = (tracker->centerPos.x + tracker->velocity.x) * scale;
            float predY = (tracker->centerPos.y + tracker->velocity.y) * scale;
            float w = tracker->targetRect.width * scale;
            float h = tracker->targetRect.height * scale;
            
            cv::Rect predictedBox((int)(predX - w/2), (int)(predY - h/2), (int)w, (int)h);
            predictedBox &= cv::Rect(0, 0, frame.cols, frame.rows);
            
            if (predictedBox.area() > 0) {
                wrapper->cvTracker = cv::TrackerCSRT::create();
                wrapper->cvTracker->init(frame, predictedBox);
                bbox = predictedBox;
                ok = true;
                wrapper->lostFrameCount++;
            } else {
                ok = false;
            }
        } 
        else if (ok) {
            wrapper->lostFrameCount = 0;
        }

        if (ok) {
            float invScale = 1.0f / scale;
            
            float realX = bbox.x * invScale;
            float realY = bbox.y * invScale;
            float realW = bbox.width * invScale;
            float realH = bbox.height * invScale;

            Vector2 newCenter = Vector2{ 
                realX + realW / 2.0f, 
                realY + realH / 2.0f 
            };

            float margin = 10.0f;
            if (newCenter.x < margin || newCenter.x > video->width - margin ||
                newCenter.y < margin || newCenter.y > video->height - margin) {
                tracker->state = TRACKER_IDLE;
                tracker->needsToAdvance = false;
                printf("[OpenCV] Bord ecran atteint.\n");
                return;
            }

            float dx = newCenter.x - tracker->centerPos.x;
            float dy = newCenter.y - tracker->centerPos.y;
            float dist = sqrtf(dx*dx + dy*dy);
            float maxJump = (float)video->width * 0.15f; 
            if (maxJump < 150.0f) maxJump = 150.0f;

            if (dist > maxJump && wrapper->lostFrameCount == 0) {
                tracker->needsToAdvance = true; 
                return; 
            }

            tracker->velocity.x = dx;
            tracker->velocity.y = dy;
            tracker->centerPos = newCenter;
            
            tracker->targetRect.x = realX;
            tracker->targetRect.y = realY;
            tracker->targetRect.width = realW;
            tracker->targetRect.height = realH;
            
            Tracking_AddPoint(ts, video->currentTime, tracker->centerPos);
            
            double timePerFrame = 1.0 / video->fps;
            
            if (video->currentTime + timePerFrame >= video->durationSec - 0.001) {
                tracker->state = TRACKER_IDLE;
                tracker->needsToAdvance = false;
                printf("[OpenCV] Fin video atteinte (Derniere frame).\n");
            } else {
                tracker->needsToAdvance = true;
            }
            
        } else {
            tracker->state = TRACKER_LOST;
            tracker->needsToAdvance = false;
        }

    } catch (const cv::Exception& e) {
        printf("[OpenCV CRASH EVITE] %s\n", e.what());
        tracker->state = TRACKER_IDLE;
        tracker->needsToAdvance = false;
    }
}

void AutoTracker_Free(AutoTracker *tracker) {
    if (tracker->internal_ptr) {
        delete (TrackerWrapper*)tracker->internal_ptr;
        tracker->internal_ptr = nullptr;
    }
}

}