// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// @generated

#ifndef ISDK_API_HPP
#define ISDK_API_HPP

#include <memory>
#include "isdk_api.h"

namespace isdk { namespace api {
template <class TDerived, class TBase>
TBase* tryInitWithCast(TDerived* handle, void (*castMethod)(TDerived*, TBase**)) {
  TBase* base{};
  if (handle) {
    castMethod(handle, &base);
  }
  return base;
}

/**
 * A unique pointer that owns the lifecycle of the handle it is constructed with.
 */
template <class THelperType, class THandle>
class ObjectPtr {
 public:
  explicit ObjectPtr(void (*destroyMethod)(THandle*) = &THelperType::destroy)
      : destroyMethod_(destroyMethod),
        objectHelper_(THelperType(nullptr)),
        objectInstance_({nullptr, destroyMethod}) {}
  ObjectPtr(THandle* handle_, void (*destroyMethod)(THandle*) = &THelperType::destroy)
      : destroyMethod_(destroyMethod),
        objectHelper_(THelperType(handle_)),
        objectInstance_({handle_, destroyMethod}) {}

  // Disallow copying
  ObjectPtr(const ObjectPtr&) = delete;
  ObjectPtr& operator=(const ObjectPtr&) = delete;

  // Allow moving
  ObjectPtr(ObjectPtr&&) = default;
  ObjectPtr& operator=(ObjectPtr&&) = default;

  THelperType* operator->() {
    return &objectHelper_;
  }

  const THelperType* operator->() const {
    return &objectHelper_;
  }

  const THelperType& operator*() const {
    return objectHelper_;
  }

  THelperType& operator*() {
    return objectHelper_;
  }

  const THelperType& Get() const {
    return objectHelper_;
  }

  THelperType& Get() {
    return objectHelper_;
  }

  bool IsValid() const {
    return objectInstance_.get() != nullptr;
  }

  void Reset(THandle* Handle = nullptr) {
    objectHelper_ = THelperType(Handle);
    objectInstance_ = {Handle, destroyMethod_};
  }

 private:
  void (*destroyMethod_)(THandle*);
  THelperType objectHelper_;
  std::unique_ptr<THandle, void (*)(THandle*)> objectInstance_;
};
class ApiContext {
 public:
  virtual ~ApiContext() = default;
  [[nodiscard]] virtual isdk_ApiContext* getApiContextHandle() const = 0;
  static unsigned long long getVersion(char* outString, const int outStringCount) {
    unsigned long long retVal{};
    isdk_ApiContext_getVersion(outString, outStringCount, &retVal);
    return retVal;
  }
};
class IActiveState {
 public:
  virtual ~IActiveState() = default;
  [[nodiscard]] virtual isdk_IActiveState* getIActiveStateHandle() const = 0;
  unsigned char isActive() {
    unsigned char retVal{};
    isdk_IActiveState_isActive(getIActiveStateHandle(), &retVal);
    return retVal;
  }
};
class IBounds {
 public:
  virtual ~IBounds() = default;
  [[nodiscard]] virtual isdk_IBounds* getIBoundsHandle() const = 0;
  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_IBounds_getBounds(getIBoundsHandle(), &retVal);
    return retVal;
  }
};
class ICandidateProperty {
 public:
  virtual ~ICandidateProperty() = default;
  [[nodiscard]] virtual isdk_ICandidateProperty* getICandidatePropertyHandle() const = 0;
};
class IDeltaTimeUpdate {
 public:
  virtual ~IDeltaTimeUpdate() = default;
  [[nodiscard]] virtual isdk_IDeltaTimeUpdate* getIDeltaTimeUpdateHandle() const = 0;
  void update(float deltaTime) {
    isdk_IDeltaTimeUpdate_update(getIDeltaTimeUpdateHandle(), deltaTime);
  }
};
class DigitRecognizer : public IActiveState, public IDeltaTimeUpdate {
 public:
  virtual ~DigitRecognizer() = default;
  [[nodiscard]] virtual isdk_DigitRecognizer* getDigitRecognizerHandle() const = 0;
  void update(float deltaTime) {
    isdk_DigitRecognizer_update(getDigitRecognizerHandle(), deltaTime);
  }

  isdk_DigitRecognizer_UpdateResult getUpdateResult() {
    isdk_DigitRecognizer_UpdateResult retVal{};
    isdk_DigitRecognizer_getUpdateResult(getDigitRecognizerHandle(), &retVal);
    return retVal;
  }

  unsigned char isActive() {
    unsigned char retVal{};
    isdk_DigitRecognizer_isActive(getDigitRecognizerHandle(), &retVal);
    return retVal;
  }

  float getRawValueNormalized() {
    float retVal{};
    isdk_DigitRecognizer_getRawValueNormalized(getDigitRecognizerHandle(), &retVal);
    return retVal;
  }
};
class IEnable {
 public:
  virtual ~IEnable() = default;
  [[nodiscard]] virtual isdk_IEnable* getIEnableHandle() const = 0;
  void onEnable() {
    isdk_IEnable_onEnable(getIEnableHandle());
  }

  void onDisable() {
    isdk_IEnable_onDisable(getIEnableHandle());
  }
};
class IFingerPositions {
 public:
  virtual ~IFingerPositions() = default;
  [[nodiscard]] virtual isdk_IFingerPositions* getIFingerPositionsHandle() const = 0;
  ovrpVector3f getFingerPosition(isdk_FingerType type, isdk_FingerJoint joint) {
    ovrpVector3f retVal{};
    isdk_IFingerPositions_getFingerPosition(getIFingerPositionsHandle(), type, joint, &retVal);
    return retVal;
  }
};
class IHandDataSource {
 public:
  virtual ~IHandDataSource() = default;
  [[nodiscard]] virtual isdk_IHandDataSource* getIHandDataSourceHandle() const = 0;
  void getData(isdk_HandData* retVal) {
    isdk_IHandDataSource_getData(getIHandDataSourceHandle(), retVal);
  }

  void getJoints(ovrpQuatf* retVal, const int retValCount, int* retValCountOut) {
    isdk_IHandDataSource_getJoints(getIHandDataSourceHandle(), retVal, retValCount, retValCountOut);
  }

  ovrpPosef getRootPose() {
    ovrpPosef retVal{};
    isdk_IHandDataSource_getRootPose(getIHandDataSourceHandle(), &retVal);
    return retVal;
  }

  isdk_DataSourceUpdateDataResult updateData() {
    isdk_DataSourceUpdateDataResult retVal{};
    isdk_IHandDataSource_updateData(getIHandDataSourceHandle(), &retVal);
    return retVal;
  }
};
class IHandDataModifier : public IHandDataSource {
 public:
  virtual ~IHandDataModifier() = default;
  [[nodiscard]] virtual isdk_IHandDataModifier* getIHandDataModifierHandle() const = 0;
  void setFromDataSource(isdk_IHandDataSource* fromDataSource) {
    isdk_IHandDataModifier_setFromDataSource(getIHandDataModifierHandle(), fromDataSource);
  }

  isdk_IHandDataSource* getFromDataSource() {
    isdk_IHandDataSource* retVal{};
    isdk_IHandDataModifier_getFromDataSource(getIHandDataModifierHandle(), &retVal);
    return retVal;
  }

  void setRecursiveUpdate(unsigned char recursiveUpdate) {
    isdk_IHandDataModifier_setRecursiveUpdate(getIHandDataModifierHandle(), recursiveUpdate);
  }

  unsigned char getRecursiveUpdate() {
    unsigned char retVal{};
    isdk_IHandDataModifier_getRecursiveUpdate(getIHandDataModifierHandle(), &retVal);
    return retVal;
  }
};
class IHandedness {
 public:
  virtual ~IHandedness() = default;
  [[nodiscard]] virtual isdk_IHandedness* getIHandednessHandle() const = 0;
  isdk_Handedness getHandedness() {
    isdk_Handedness retVal{};
    isdk_IHandedness_getHandedness(getIHandednessHandle(), &retVal);
    return retVal;
  }
};
class IHasSelector {
 public:
  virtual ~IHasSelector() = default;
  [[nodiscard]] virtual isdk_IHasSelector* getIHasSelectorHandle() const = 0;
  isdk_ISelector* getSelector() {
    isdk_ISelector* retVal{};
    isdk_IHasSelector_getSelector(getIHasSelectorHandle(), &retVal);
    return retVal;
  }

  void setSelector(isdk_ISelector* newSelector) {
    isdk_IHasSelector_setSelector(getIHasSelectorHandle(), newSelector);
  }
};
class IInteractable {
 public:
  virtual ~IInteractable() = default;
  [[nodiscard]] virtual isdk_IInteractable* getIInteractableHandle() const = 0;
  void enable() {
    isdk_IInteractable_enable(getIInteractableHandle());
  }

  void disable() {
    isdk_IInteractable_disable(getIInteractableHandle());
  }

  isdk_InteractableState getState() {
    isdk_InteractableState retVal{};
    isdk_IInteractable_getState(getIInteractableHandle(), &retVal);
    return retVal;
  }

  unsigned long long getInteractors(isdk_IInteractor** interactors, const int interactorsCount) {
    unsigned long long retVal{};
    isdk_IInteractable_getInteractors(
        getIInteractableHandle(), interactors, interactorsCount, &retVal);
    return retVal;
  }

  unsigned long long getSelectingInteractors(
      isdk_IInteractor** interactors,
      const int interactorsCount) {
    unsigned long long retVal{};
    isdk_IInteractable_getSelectingInteractors(
        getIInteractableHandle(), interactors, interactorsCount, &retVal);
    return retVal;
  }

  int getInteractorsCount() {
    int retVal{};
    isdk_IInteractable_getInteractorsCount(getIInteractableHandle(), &retVal);
    return retVal;
  }

  int getSelectingInteractorsCount() {
    int retVal{};
    isdk_IInteractable_getSelectingInteractorsCount(getIInteractableHandle(), &retVal);
    return retVal;
  }

  isdk_IPayload* getPayload() {
    isdk_IPayload* retVal{};
    isdk_IInteractable_getPayload(getIInteractableHandle(), &retVal);
    return retVal;
  }

  isdk_IPayload* getPayloadSharedPtr() {
    isdk_IPayload* retVal{};
    isdk_IInteractable_getPayloadSharedPtr(getIInteractableHandle(), &retVal);
    return retVal;
  }

  void setPayload(isdk_IPayload* payload) {
    isdk_IInteractable_setPayload(getIInteractableHandle(), payload);
  }
};
class IInteractorView {
 public:
  virtual ~IInteractorView() = default;
  [[nodiscard]] virtual isdk_IInteractorView* getIInteractorViewHandle() const = 0;
  int getIdentifier() {
    int retVal{};
    isdk_IInteractorView_getIdentifier(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  isdk_IPayload* getPayload() {
    isdk_IPayload* retVal{};
    isdk_IInteractorView_getPayload(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  isdk_IPayload* getPayloadSharedPtr() {
    isdk_IPayload* retVal{};
    isdk_IInteractorView_getPayloadSharedPtr(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  void setPayload(isdk_IPayload* payload) {
    isdk_IInteractorView_setPayload(getIInteractorViewHandle(), payload);
  }

  unsigned char hasCandidate() {
    unsigned char retVal{};
    isdk_IInteractorView_hasCandidate(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  isdk_ICandidateProperty* getCandidateProperties() {
    isdk_ICandidateProperty* retVal{};
    isdk_IInteractorView_getCandidateProperties(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  unsigned char hasInteractable() {
    unsigned char retVal{};
    isdk_IInteractorView_hasInteractable(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  unsigned char hasSelectedInteractable() {
    unsigned char retVal{};
    isdk_IInteractorView_hasSelectedInteractable(getIInteractorViewHandle(), &retVal);
    return retVal;
  }

  isdk_InteractorState getState() {
    isdk_InteractorState retVal{};
    isdk_IInteractorView_getState(getIInteractorViewHandle(), &retVal);
    return retVal;
  }
};
class IMovementProvider {
 public:
  virtual ~IMovementProvider() = default;
  [[nodiscard]] virtual isdk_IMovementProvider* getIMovementProviderHandle() const = 0;
};
class IPayload {
 public:
  virtual ~IPayload() = default;
  [[nodiscard]] virtual isdk_IPayload* getIPayloadHandle() const = 0;
};
class IPointable {
 public:
  virtual ~IPointable() = default;
  [[nodiscard]] virtual isdk_IPointable* getIPointableHandle() const = 0;
};
class IPointableElement : public IPointable {
 public:
  virtual ~IPointableElement() = default;
  [[nodiscard]] virtual isdk_IPointableElement* getIPointableElementHandle() const = 0;
  void processPointerEvent(const isdk_PointerEvent* paramVar0) {
    isdk_IPointableElement_processPointerEvent(getIPointableElementHandle(), paramVar0);
  }
};
class IPose {
 public:
  virtual ~IPose() = default;
  [[nodiscard]] virtual isdk_IPose* getIPoseHandle() const = 0;
  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_IPose_getPose(getIPoseHandle(), &retVal);
    return retVal;
  }
};
class IPosition {
 public:
  virtual ~IPosition() = default;
  [[nodiscard]] virtual isdk_IPosition* getIPositionHandle() const = 0;
  ovrpVector3f getPosition() {
    ovrpVector3f retVal{};
    isdk_IPosition_getPosition(getIPositionHandle(), &retVal);
    return retVal;
  }
};
class ISelector {
 public:
  virtual ~ISelector() = default;
  [[nodiscard]] virtual isdk_ISelector* getISelectorHandle() const = 0;
};
class ISurface : public IPose, public IBounds {
 public:
  virtual ~ISurface() = default;
  [[nodiscard]] virtual isdk_ISurface* getISurfaceHandle() const = 0;
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_ISurface_raycast(getISurfaceHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_ISurface_closestSurfacePoint(getISurfaceHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }
};
class ISurfacePatch : public ISurface {
 public:
  virtual ~ISurfacePatch() = default;
  [[nodiscard]] virtual isdk_ISurfacePatch* getISurfacePatchHandle() const = 0;
  isdk_ISurface* getBackingSurface() {
    isdk_ISurface* retVal{};
    isdk_ISurfacePatch_getBackingSurface(getISurfacePatchHandle(), &retVal);
    return retVal;
  }
};
class IThumbPositions {
 public:
  virtual ~IThumbPositions() = default;
  [[nodiscard]] virtual isdk_IThumbPositions* getIThumbPositionsHandle() const = 0;
  ovrpVector3f getThumbPosition(isdk_ThumbJoint joint) {
    ovrpVector3f retVal{};
    isdk_IThumbPositions_getThumbPosition(getIThumbPositionsHandle(), joint, &retVal);
    return retVal;
  }
};
class ITimeProvider {
 public:
  virtual ~ITimeProvider() = default;
  [[nodiscard]] virtual isdk_ITimeProvider* getITimeProviderHandle() const = 0;
};
class IUpdate {
 public:
  virtual ~IUpdate() = default;
  [[nodiscard]] virtual isdk_IUpdate* getIUpdateHandle() const = 0;
  void update() {
    isdk_IUpdate_update(getIUpdateHandle());
  }
};
class IUpdateDriver {
 public:
  virtual ~IUpdateDriver() = default;
  [[nodiscard]] virtual isdk_IUpdateDriver* getIUpdateDriverHandle() const = 0;
  unsigned char getIsRootDriver() {
    unsigned char retVal{};
    isdk_IUpdateDriver_getIsRootDriver(getIUpdateDriverHandle(), &retVal);
    return retVal;
  }

  void setIsRootDriver(unsigned char value) {
    isdk_IUpdateDriver_setIsRootDriver(getIUpdateDriverHandle(), value);
  }

  void drive() {
    isdk_IUpdateDriver_drive(getIUpdateDriverHandle());
  }
};
class IInteractor : public IInteractorView, public IUpdate, public IUpdateDriver {
 public:
  virtual ~IInteractor() = default;
  [[nodiscard]] virtual isdk_IInteractor* getIInteractorHandle() const = 0;
  void preprocess() {
    isdk_IInteractor_preprocess(getIInteractorHandle());
  }

  void process() {
    isdk_IInteractor_process(getIInteractorHandle());
  }

  void postprocess() {
    isdk_IInteractor_postprocess(getIInteractorHandle());
  }

  void processCandidate() {
    isdk_IInteractor_processCandidate(getIInteractorHandle());
  }

  void enable() {
    isdk_IInteractor_enable(getIInteractorHandle());
  }

  void disable() {
    isdk_IInteractor_disable(getIInteractorHandle());
  }

  void hover() {
    isdk_IInteractor_hover(getIInteractorHandle());
  }

  void unhover() {
    isdk_IInteractor_unhover(getIInteractorHandle());
  }

  void select() {
    isdk_IInteractor_select(getIInteractorHandle());
  }

  void unselect() {
    isdk_IInteractor_unselect(getIInteractorHandle());
  }

  unsigned char shouldHover() {
    unsigned char retVal{};
    isdk_IInteractor_shouldHover(getIInteractorHandle(), &retVal);
    return retVal;
  }

  unsigned char shouldUnhover() {
    unsigned char retVal{};
    isdk_IInteractor_shouldUnhover(getIInteractorHandle(), &retVal);
    return retVal;
  }

  unsigned char shouldSelect() {
    unsigned char retVal{};
    isdk_IInteractor_shouldSelect(getIInteractorHandle(), &retVal);
    return retVal;
  }

  unsigned char shouldUnselect() {
    unsigned char retVal{};
    isdk_IInteractor_shouldUnselect(getIInteractorHandle(), &retVal);
    return retVal;
  }

  isdk_IInteractable* getInteractablePtr() {
    isdk_IInteractable* retVal{};
    isdk_IInteractor_getInteractablePtr(getIInteractorHandle(), &retVal);
    return retVal;
  }

  isdk_IInteractable* getSelectedInteractablePtr() {
    isdk_IInteractable* retVal{};
    isdk_IInteractor_getSelectedInteractablePtr(getIInteractorHandle(), &retVal);
    return retVal;
  }
};
class IWristPosition {
 public:
  virtual ~IWristPosition() = default;
  [[nodiscard]] virtual isdk_IWristPosition* getIWristPositionHandle() const = 0;
  ovrpVector3f getWristPosition() {
    ovrpVector3f retVal{};
    isdk_IWristPosition_getWristPosition(getIWristPositionHandle(), &retVal);
    return retVal;
  }
};
class IHandPosition : public IFingerPositions,
                      public IThumbPositions,
                      public IWristPosition,
                      public IHandedness {
 public:
  virtual ~IHandPosition() = default;
  [[nodiscard]] virtual isdk_IHandPosition* getIHandPositionHandle() const = 0;
};
class IHandPositionFrame : public IHandPosition {
 public:
  virtual ~IHandPositionFrame() = default;
  [[nodiscard]] virtual isdk_IHandPositionFrame* getIHandPositionFrameHandle() const = 0;
};
class ActiveStateBool final : public IActiveState {
 public:
  explicit ActiveStateBool(isdk_ActiveStateBool* handle = nullptr)
      : asActiveStateBool_(handle),
        asIActiveState_(tryInitWithCast<isdk_ActiveStateBool, isdk_IActiveState>(
            handle,
            &isdk_ActiveStateBool_castToIActiveState)) {}
  static ObjectPtr<ActiveStateBool, isdk_ActiveStateBool> create() {
    isdk_ActiveStateBool* retVal{};
    isdk_ActiveStateBool_create(&retVal);
    return ObjectPtr<ActiveStateBool, isdk_ActiveStateBool>(retVal, &isdk_ActiveStateBool_destroy);
  }
  static void destroy(isdk_ActiveStateBool* ptr) {
    isdk_ActiveStateBool_destroy(ptr);
  }
  unsigned char isActive() {
    unsigned char retVal{};
    isdk_ActiveStateBool_isActive(getActiveStateBoolHandle(), &retVal);
    return retVal;
  }

  void setActive(const bool value) {
    isdk_ActiveStateBool_setActive(getActiveStateBoolHandle(), value);
  }
  [[nodiscard]] isdk_ActiveStateBool* getActiveStateBoolHandle() const {
    return asActiveStateBool_;
  }
  operator isdk_ActiveStateBool*() const {
    return asActiveStateBool_;
  }
  [[nodiscard]] isdk_IActiveState* getIActiveStateHandle() const override {
    return asIActiveState_;
  }
  operator isdk_IActiveState*() const {
    return asIActiveState_;
  }

 protected:
  isdk_ActiveStateBool* asActiveStateBool_;
  isdk_IActiveState* asIActiveState_;
};
using ActiveStateBoolPtr = ObjectPtr<ActiveStateBool, isdk_ActiveStateBool>;

class ClippedPlaneSurface final : public ISurfacePatch {
 public:
  explicit ClippedPlaneSurface(isdk_ClippedPlaneSurface* handle = nullptr)
      : asClippedPlaneSurface_(handle),
        asIPose_(tryInitWithCast<isdk_ClippedPlaneSurface, isdk_IPose>(
            handle,
            &isdk_ClippedPlaneSurface_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_ClippedPlaneSurface, isdk_IBounds>(
            handle,
            &isdk_ClippedPlaneSurface_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_ClippedPlaneSurface, isdk_ISurface>(
            handle,
            &isdk_ClippedPlaneSurface_castToISurface)),
        asISurfacePatch_(tryInitWithCast<isdk_ClippedPlaneSurface, isdk_ISurfacePatch>(
            handle,
            &isdk_ClippedPlaneSurface_castToISurfacePatch)) {}
  static ObjectPtr<ClippedPlaneSurface, isdk_ClippedPlaneSurface> create(
      isdk_PointablePlane* planeSurface) {
    isdk_ClippedPlaneSurface* retVal{};
    isdk_ClippedPlaneSurface_create(planeSurface, &retVal);
    return ObjectPtr<ClippedPlaneSurface, isdk_ClippedPlaneSurface>(
        retVal, &isdk_ClippedPlaneSurface_destroy);
  }
  static ObjectPtr<ClippedPlaneSurface, isdk_ClippedPlaneSurface> createWithClippers(
      isdk_PointablePlane* planeSurface,
      isdk_BoundsClipper* clippers,
      const int clippersCount) {
    isdk_ClippedPlaneSurface* retVal{};
    isdk_ClippedPlaneSurface_createWithClippers(planeSurface, clippers, clippersCount, &retVal);
    return ObjectPtr<ClippedPlaneSurface, isdk_ClippedPlaneSurface>(
        retVal, &isdk_ClippedPlaneSurface_destroy);
  }
  static void destroy(isdk_ClippedPlaneSurface* ptr) {
    isdk_ClippedPlaneSurface_destroy(ptr);
  }
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_ClippedPlaneSurface_raycast(
        getClippedPlaneSurfaceHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_ClippedPlaneSurface_closestSurfacePoint(
        getClippedPlaneSurfaceHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  void setPose(const ovrpPosef* pose) {
    isdk_ClippedPlaneSurface_setPose(getClippedPlaneSurfaceHandle(), pose);
  }

  ovrpVector3f getNormal() {
    ovrpVector3f retVal{};
    isdk_ClippedPlaneSurface_getNormal(getClippedPlaneSurfaceHandle(), &retVal);
    return retVal;
  }

  void setSize(const ovrpVector2f* newSize) {
    isdk_ClippedPlaneSurface_setSize(getClippedPlaneSurfaceHandle(), newSize);
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_ClippedPlaneSurface_getPose(getClippedPlaneSurfaceHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_ClippedPlaneSurface_getBounds(getClippedPlaneSurfaceHandle(), &retVal);
    return retVal;
  }

  isdk_ISurface* getBackingSurface() {
    isdk_ISurface* retVal{};
    isdk_ClippedPlaneSurface_getBackingSurface(getClippedPlaneSurfaceHandle(), &retVal);
    return retVal;
  }

  void getClippers(isdk_BoundsClipper* retVal, const int retValCount, int* retValCountOut) {
    isdk_ClippedPlaneSurface_getClippers(
        getClippedPlaneSurfaceHandle(), retVal, retValCount, retValCountOut);
  }

  void setClippers(isdk_BoundsClipper* clippers, const int clippersCount) {
    isdk_ClippedPlaneSurface_setClippers(getClippedPlaneSurfaceHandle(), clippers, clippersCount);
  }
  [[nodiscard]] isdk_ClippedPlaneSurface* getClippedPlaneSurfaceHandle() const {
    return asClippedPlaneSurface_;
  }
  operator isdk_ClippedPlaneSurface*() const {
    return asClippedPlaneSurface_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }
  [[nodiscard]] isdk_ISurfacePatch* getISurfacePatchHandle() const override {
    return asISurfacePatch_;
  }
  operator isdk_ISurfacePatch*() const {
    return asISurfacePatch_;
  }

 protected:
  isdk_ClippedPlaneSurface* asClippedPlaneSurface_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
  isdk_ISurfacePatch* asISurfacePatch_;
};
using ClippedPlaneSurfacePtr = ObjectPtr<ClippedPlaneSurface, isdk_ClippedPlaneSurface>;

class CylinderSurface final : public ISurface {
 public:
  explicit CylinderSurface(isdk_CylinderSurface* handle = nullptr)
      : asCylinderSurface_(handle),
        asIPose_(tryInitWithCast<isdk_CylinderSurface, isdk_IPose>(
            handle,
            &isdk_CylinderSurface_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_CylinderSurface, isdk_IBounds>(
            handle,
            &isdk_CylinderSurface_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_CylinderSurface, isdk_ISurface>(
            handle,
            &isdk_CylinderSurface_castToISurface)) {}
  static ObjectPtr<CylinderSurface, isdk_CylinderSurface> create() {
    isdk_CylinderSurface* retVal{};
    isdk_CylinderSurface_create(&retVal);
    return ObjectPtr<CylinderSurface, isdk_CylinderSurface>(retVal, &isdk_CylinderSurface_destroy);
  }
  static ObjectPtr<CylinderSurface, isdk_CylinderSurface> createWithCylinder(
      const ovrpPosef* pose,
      float height,
      float radius,
      isdk_NormalFacing normalFacing) {
    isdk_CylinderSurface* retVal{};
    isdk_CylinderSurface_createWithCylinder(pose, height, radius, normalFacing, &retVal);
    return ObjectPtr<CylinderSurface, isdk_CylinderSurface>(retVal, &isdk_CylinderSurface_destroy);
  }
  static void destroy(isdk_CylinderSurface* ptr) {
    isdk_CylinderSurface_destroy(ptr);
  }
  void
  setCylinder(const ovrpPosef* pose, float height, float radius, isdk_NormalFacing normalFacing) {
    isdk_CylinderSurface_setCylinder(
        getCylinderSurfaceHandle(), pose, height, radius, normalFacing);
  }

  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_CylinderSurface_raycast(getCylinderSurfaceHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_CylinderSurface_closestSurfacePoint(
        getCylinderSurfaceHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_CylinderSurface_getPose(getCylinderSurfaceHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_CylinderSurface_getBounds(getCylinderSurfaceHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_CylinderSurface* getCylinderSurfaceHandle() const {
    return asCylinderSurface_;
  }
  operator isdk_CylinderSurface*() const {
    return asCylinderSurface_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }

 protected:
  isdk_CylinderSurface* asCylinderSurface_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
};
using CylinderSurfacePtr = ObjectPtr<CylinderSurface, isdk_CylinderSurface>;

class DummyHandSource final : public IHandDataSource {
 public:
  explicit DummyHandSource(isdk_DummyHandSource* handle = nullptr)
      : asDummyHandSource_(handle),
        asIHandDataSource_(tryInitWithCast<isdk_DummyHandSource, isdk_IHandDataSource>(
            handle,
            &isdk_DummyHandSource_castToIHandDataSource)) {}
  static ObjectPtr<DummyHandSource, isdk_DummyHandSource> create(int numJoints) {
    isdk_DummyHandSource* retVal{};
    isdk_DummyHandSource_create(numJoints, &retVal);
    return ObjectPtr<DummyHandSource, isdk_DummyHandSource>(retVal, &isdk_DummyHandSource_destroy);
  }
  static void destroy(isdk_DummyHandSource* ptr) {
    isdk_DummyHandSource_destroy(ptr);
  }
  void getData(isdk_HandData* retVal) {
    isdk_DummyHandSource_getData(getDummyHandSourceHandle(), retVal);
  }

  void getJoints(ovrpQuatf* retVal, const int retValCount, int* retValCountOut) {
    isdk_DummyHandSource_getJoints(getDummyHandSourceHandle(), retVal, retValCount, retValCountOut);
  }

  ovrpPosef getRootPose() {
    ovrpPosef retVal{};
    isdk_DummyHandSource_getRootPose(getDummyHandSourceHandle(), &retVal);
    return retVal;
  }

  isdk_DataSourceUpdateDataResult updateData() {
    isdk_DataSourceUpdateDataResult retVal{};
    isdk_DummyHandSource_updateData(getDummyHandSourceHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_DummyHandSource* getDummyHandSourceHandle() const {
    return asDummyHandSource_;
  }
  operator isdk_DummyHandSource*() const {
    return asDummyHandSource_;
  }
  [[nodiscard]] isdk_IHandDataSource* getIHandDataSourceHandle() const override {
    return asIHandDataSource_;
  }
  operator isdk_IHandDataSource*() const {
    return asIHandDataSource_;
  }

 protected:
  isdk_DummyHandSource* asDummyHandSource_;
  isdk_IHandDataSource* asIHandDataSource_;
};
using DummyHandSourcePtr = ObjectPtr<DummyHandSource, isdk_DummyHandSource>;

class ExternalHandPositionFrame final : public IHandPositionFrame {
 public:
  explicit ExternalHandPositionFrame(isdk_ExternalHandPositionFrame* handle = nullptr)
      : asExternalHandPositionFrame_(handle),
        asIFingerPositions_(tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IFingerPositions>(
            handle,
            &isdk_ExternalHandPositionFrame_castToIFingerPositions)),
        asIThumbPositions_(tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IThumbPositions>(
            handle,
            &isdk_ExternalHandPositionFrame_castToIThumbPositions)),
        asIWristPosition_(tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IWristPosition>(
            handle,
            &isdk_ExternalHandPositionFrame_castToIWristPosition)),
        asIHandedness_(tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IHandedness>(
            handle,
            &isdk_ExternalHandPositionFrame_castToIHandedness)),
        asIHandPosition_(tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IHandPosition>(
            handle,
            &isdk_ExternalHandPositionFrame_castToIHandPosition)),
        asIHandPositionFrame_(
            tryInitWithCast<isdk_ExternalHandPositionFrame, isdk_IHandPositionFrame>(
                handle,
                &isdk_ExternalHandPositionFrame_castToIHandPositionFrame)) {}
  static ObjectPtr<ExternalHandPositionFrame, isdk_ExternalHandPositionFrame> create(
      isdk_Handedness handedness,
      const isdk_ExternalHandPositionFrame_ThumbJointMapping* thumbMappings,
      const int thumbMappingsCount,
      const isdk_ExternalHandPositionFrame_FingerJointMapping* fingerMappings,
      const int fingerMappingsCount,
      unsigned long long jointDataSize) {
    isdk_ExternalHandPositionFrame* retVal{};
    isdk_ExternalHandPositionFrame_create(
        handedness,
        thumbMappings,
        thumbMappingsCount,
        fingerMappings,
        fingerMappingsCount,
        jointDataSize,
        &retVal);
    return ObjectPtr<ExternalHandPositionFrame, isdk_ExternalHandPositionFrame>(
        retVal, &isdk_ExternalHandPositionFrame_destroy);
  }
  static void destroy(isdk_ExternalHandPositionFrame* ptr) {
    isdk_ExternalHandPositionFrame_destroy(ptr);
  }
  void setJointData(ovrpVector3f* jointData, const int jointDataCount) {
    isdk_ExternalHandPositionFrame_setJointData(
        getExternalHandPositionFrameHandle(), jointData, jointDataCount);
  }

  void setWristData(const ovrpVector3f* wristPositionWorld) {
    isdk_ExternalHandPositionFrame_setWristData(
        getExternalHandPositionFrameHandle(), wristPositionWorld);
  }

  ovrpVector3f getFingerPosition(isdk_FingerType type, isdk_FingerJoint joint) {
    ovrpVector3f retVal{};
    isdk_ExternalHandPositionFrame_getFingerPosition(
        getExternalHandPositionFrameHandle(), type, joint, &retVal);
    return retVal;
  }

  ovrpVector3f getThumbPosition(isdk_ThumbJoint joint) {
    ovrpVector3f retVal{};
    isdk_ExternalHandPositionFrame_getThumbPosition(
        getExternalHandPositionFrameHandle(), joint, &retVal);
    return retVal;
  }

  ovrpVector3f getWristPosition() {
    ovrpVector3f retVal{};
    isdk_ExternalHandPositionFrame_getWristPosition(getExternalHandPositionFrameHandle(), &retVal);
    return retVal;
  }

  isdk_Handedness getHandedness() {
    isdk_Handedness retVal{};
    isdk_ExternalHandPositionFrame_getHandedness(getExternalHandPositionFrameHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_ExternalHandPositionFrame* getExternalHandPositionFrameHandle() const {
    return asExternalHandPositionFrame_;
  }
  operator isdk_ExternalHandPositionFrame*() const {
    return asExternalHandPositionFrame_;
  }
  [[nodiscard]] isdk_IFingerPositions* getIFingerPositionsHandle() const override {
    return asIFingerPositions_;
  }
  operator isdk_IFingerPositions*() const {
    return asIFingerPositions_;
  }
  [[nodiscard]] isdk_IThumbPositions* getIThumbPositionsHandle() const override {
    return asIThumbPositions_;
  }
  operator isdk_IThumbPositions*() const {
    return asIThumbPositions_;
  }
  [[nodiscard]] isdk_IWristPosition* getIWristPositionHandle() const override {
    return asIWristPosition_;
  }
  operator isdk_IWristPosition*() const {
    return asIWristPosition_;
  }
  [[nodiscard]] isdk_IHandedness* getIHandednessHandle() const override {
    return asIHandedness_;
  }
  operator isdk_IHandedness*() const {
    return asIHandedness_;
  }
  [[nodiscard]] isdk_IHandPosition* getIHandPositionHandle() const override {
    return asIHandPosition_;
  }
  operator isdk_IHandPosition*() const {
    return asIHandPosition_;
  }
  [[nodiscard]] isdk_IHandPositionFrame* getIHandPositionFrameHandle() const override {
    return asIHandPositionFrame_;
  }
  operator isdk_IHandPositionFrame*() const {
    return asIHandPositionFrame_;
  }

 protected:
  isdk_ExternalHandPositionFrame* asExternalHandPositionFrame_;
  isdk_IFingerPositions* asIFingerPositions_;
  isdk_IThumbPositions* asIThumbPositions_;
  isdk_IWristPosition* asIWristPosition_;
  isdk_IHandedness* asIHandedness_;
  isdk_IHandPosition* asIHandPosition_;
  isdk_IHandPositionFrame* asIHandPositionFrame_;
};
using ExternalHandPositionFramePtr =
    ObjectPtr<ExternalHandPositionFrame, isdk_ExternalHandPositionFrame>;

class ExternalHandSource final : public IHandDataSource {
 public:
  explicit ExternalHandSource(isdk_ExternalHandSource* handle = nullptr)
      : asExternalHandSource_(handle),
        asIHandDataSource_(tryInitWithCast<isdk_ExternalHandSource, isdk_IHandDataSource>(
            handle,
            &isdk_ExternalHandSource_castToIHandDataSource)) {}
  static ObjectPtr<ExternalHandSource, isdk_ExternalHandSource> create(int numJoints) {
    isdk_ExternalHandSource* retVal{};
    isdk_ExternalHandSource_create(numJoints, &retVal);
    return ObjectPtr<ExternalHandSource, isdk_ExternalHandSource>(
        retVal, &isdk_ExternalHandSource_destroy);
  }
  static void destroy(isdk_ExternalHandSource* ptr) {
    isdk_ExternalHandSource_destroy(ptr);
  }
  void getData(isdk_HandData* retVal) {
    isdk_ExternalHandSource_getData(getExternalHandSourceHandle(), retVal);
  }

  void getJoints(ovrpQuatf* retVal, const int retValCount, int* retValCountOut) {
    isdk_ExternalHandSource_getJoints(
        getExternalHandSourceHandle(), retVal, retValCount, retValCountOut);
  }

  ovrpPosef getRootPose() {
    ovrpPosef retVal{};
    isdk_ExternalHandSource_getRootPose(getExternalHandSourceHandle(), &retVal);
    return retVal;
  }

  isdk_DataSourceUpdateDataResult updateData() {
    isdk_DataSourceUpdateDataResult retVal{};
    isdk_ExternalHandSource_updateData(getExternalHandSourceHandle(), &retVal);
    return retVal;
  }

  isdk_DataSourceUpdateDataResult setData(const isdk_HandData* data) {
    isdk_DataSourceUpdateDataResult retVal{};
    isdk_ExternalHandSource_setData(getExternalHandSourceHandle(), data, &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_ExternalHandSource* getExternalHandSourceHandle() const {
    return asExternalHandSource_;
  }
  operator isdk_ExternalHandSource*() const {
    return asExternalHandSource_;
  }
  [[nodiscard]] isdk_IHandDataSource* getIHandDataSourceHandle() const override {
    return asIHandDataSource_;
  }
  operator isdk_IHandDataSource*() const {
    return asIHandDataSource_;
  }

 protected:
  isdk_ExternalHandSource* asExternalHandSource_;
  isdk_IHandDataSource* asIHandDataSource_;
};
using ExternalHandSourcePtr = ObjectPtr<ExternalHandSource, isdk_ExternalHandSource>;

class FingerPinchGrabRecognizer final : public IActiveState, public IDeltaTimeUpdate {
 public:
  explicit FingerPinchGrabRecognizer(isdk_FingerPinchGrabRecognizer* handle = nullptr)
      : asFingerPinchGrabRecognizer_(handle),
        asIActiveState_(tryInitWithCast<isdk_FingerPinchGrabRecognizer, isdk_IActiveState>(
            handle,
            &isdk_FingerPinchGrabRecognizer_castToIActiveState)),
        asIDeltaTimeUpdate_(tryInitWithCast<isdk_FingerPinchGrabRecognizer, isdk_IDeltaTimeUpdate>(
            handle,
            &isdk_FingerPinchGrabRecognizer_castToIDeltaTimeUpdate)) {}
  static ObjectPtr<FingerPinchGrabRecognizer, isdk_FingerPinchGrabRecognizer> create(
      isdk_IHandPositionFrame* handPositionFrame,
      isdk_FingerType fingerType) {
    isdk_FingerPinchGrabRecognizer* retVal{};
    isdk_FingerPinchGrabRecognizer_create(handPositionFrame, fingerType, &retVal);
    return ObjectPtr<FingerPinchGrabRecognizer, isdk_FingerPinchGrabRecognizer>(
        retVal, &isdk_FingerPinchGrabRecognizer_destroy);
  }
  static void destroy(isdk_FingerPinchGrabRecognizer* ptr) {
    isdk_FingerPinchGrabRecognizer_destroy(ptr);
  }
  void setInputTrackingConfidence(unsigned char isHighConfidence) {
    isdk_FingerPinchGrabRecognizer_setInputTrackingConfidence(
        getFingerPinchGrabRecognizerHandle(), isHighConfidence);
  }

  unsigned char setInputTrackingConfidenceWithData(
      ovrpVector3f wristForward,
      ovrpVector3f hmdForward) {
    unsigned char retVal{};
    isdk_FingerPinchGrabRecognizer_setInputTrackingConfidenceWithData(
        getFingerPinchGrabRecognizerHandle(), wristForward, hmdForward, &retVal);
    return retVal;
  }

  void update(float deltaTime) {
    isdk_FingerPinchGrabRecognizer_update(getFingerPinchGrabRecognizerHandle(), deltaTime);
  }

  unsigned char isActive() {
    unsigned char retVal{};
    isdk_FingerPinchGrabRecognizer_isActive(getFingerPinchGrabRecognizerHandle(), &retVal);
    return retVal;
  }

  float getPinchStrength() {
    float retVal{};
    isdk_FingerPinchGrabRecognizer_getPinchStrength(getFingerPinchGrabRecognizerHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_FingerPinchGrabRecognizer* getFingerPinchGrabRecognizerHandle() const {
    return asFingerPinchGrabRecognizer_;
  }
  operator isdk_FingerPinchGrabRecognizer*() const {
    return asFingerPinchGrabRecognizer_;
  }
  [[nodiscard]] isdk_IActiveState* getIActiveStateHandle() const override {
    return asIActiveState_;
  }
  operator isdk_IActiveState*() const {
    return asIActiveState_;
  }
  [[nodiscard]] isdk_IDeltaTimeUpdate* getIDeltaTimeUpdateHandle() const override {
    return asIDeltaTimeUpdate_;
  }
  operator isdk_IDeltaTimeUpdate*() const {
    return asIDeltaTimeUpdate_;
  }

 protected:
  isdk_FingerPinchGrabRecognizer* asFingerPinchGrabRecognizer_;
  isdk_IActiveState* asIActiveState_;
  isdk_IDeltaTimeUpdate* asIDeltaTimeUpdate_;
};
using FingerPinchGrabRecognizerPtr =
    ObjectPtr<FingerPinchGrabRecognizer, isdk_FingerPinchGrabRecognizer>;

class FingerRecognizer final : public DigitRecognizer {
 public:
  explicit FingerRecognizer(isdk_FingerRecognizer* handle = nullptr)
      : asFingerRecognizer_(handle),
        asIActiveState_(tryInitWithCast<isdk_FingerRecognizer, isdk_IActiveState>(
            handle,
            &isdk_FingerRecognizer_castToIActiveState)),
        asIDeltaTimeUpdate_(tryInitWithCast<isdk_FingerRecognizer, isdk_IDeltaTimeUpdate>(
            handle,
            &isdk_FingerRecognizer_castToIDeltaTimeUpdate)),
        asDigitRecognizer_(tryInitWithCast<isdk_FingerRecognizer, isdk_DigitRecognizer>(
            handle,
            &isdk_FingerRecognizer_castToDigitRecognizer)) {}
  static ObjectPtr<FingerRecognizer, isdk_FingerRecognizer> create(
      isdk_IHandPositionFrame* handPositionFrame,
      const isdk_DigitRangeParams* parameters,
      isdk_FingerType fingerType,
      isdk_Detection_FingerCalcType calcType) {
    isdk_FingerRecognizer* retVal{};
    isdk_FingerRecognizer_create(handPositionFrame, parameters, fingerType, calcType, &retVal);
    return ObjectPtr<FingerRecognizer, isdk_FingerRecognizer>(
        retVal, &isdk_FingerRecognizer_destroy);
  }
  static void destroy(isdk_FingerRecognizer* ptr) {
    isdk_FingerRecognizer_destroy(ptr);
  }
  static isdk_FingerRecognizer_ExpectedFingerValueRanges getExpectedFingerRawValueRanges(
      isdk_FingerType fingerType) {
    isdk_FingerRecognizer_ExpectedFingerValueRanges retVal{};
    isdk_FingerRecognizer_getExpectedFingerRawValueRanges(fingerType, &retVal);
    return retVal;
  }

  static isdk_DigitRangeParams getFingerRangeParamsFromNormalized(
      const isdk_DigitRangeParams* normalizedParams,
      isdk_FingerType fingerType,
      isdk_Detection_FingerCalcType calcType) {
    isdk_DigitRangeParams retVal{};
    isdk_FingerRecognizer_getFingerRangeParamsFromNormalized(
        normalizedParams, fingerType, calcType, &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_FingerRecognizer* getFingerRecognizerHandle() const {
    return asFingerRecognizer_;
  }
  operator isdk_FingerRecognizer*() const {
    return asFingerRecognizer_;
  }
  [[nodiscard]] isdk_IActiveState* getIActiveStateHandle() const override {
    return asIActiveState_;
  }
  operator isdk_IActiveState*() const {
    return asIActiveState_;
  }
  [[nodiscard]] isdk_IDeltaTimeUpdate* getIDeltaTimeUpdateHandle() const override {
    return asIDeltaTimeUpdate_;
  }
  operator isdk_IDeltaTimeUpdate*() const {
    return asIDeltaTimeUpdate_;
  }
  [[nodiscard]] isdk_DigitRecognizer* getDigitRecognizerHandle() const override {
    return asDigitRecognizer_;
  }
  operator isdk_DigitRecognizer*() const {
    return asDigitRecognizer_;
  }

 protected:
  isdk_FingerRecognizer* asFingerRecognizer_;
  isdk_IActiveState* asIActiveState_;
  isdk_IDeltaTimeUpdate* asIDeltaTimeUpdate_;
  isdk_DigitRecognizer* asDigitRecognizer_;
};
using FingerRecognizerPtr = ObjectPtr<FingerRecognizer, isdk_FingerRecognizer>;

class HandPokeLimiterVisual final : public IUpdate {
 public:
  explicit HandPokeLimiterVisual(isdk_HandPokeLimiterVisual* handle = nullptr)
      : asHandPokeLimiterVisual_(handle),
        asIUpdate_(tryInitWithCast<isdk_HandPokeLimiterVisual, isdk_IUpdate>(
            handle,
            &isdk_HandPokeLimiterVisual_castToIUpdate)) {}
  static ObjectPtr<HandPokeLimiterVisual, isdk_HandPokeLimiterVisual> create(
      isdk_IHandDataSource* fromDataSource,
      isdk_PokeInteractor* pokeInteractor,
      isdk_SyntheticHand* syntheticHand) {
    isdk_HandPokeLimiterVisual* retVal{};
    isdk_HandPokeLimiterVisual_create(fromDataSource, pokeInteractor, syntheticHand, &retVal);
    return ObjectPtr<HandPokeLimiterVisual, isdk_HandPokeLimiterVisual>(
        retVal, &isdk_HandPokeLimiterVisual_destroy);
  }
  static void destroy(isdk_HandPokeLimiterVisual* ptr) {
    isdk_HandPokeLimiterVisual_destroy(ptr);
  }
  [[nodiscard]] isdk_HandPokeLimiterVisual* getHandPokeLimiterVisualHandle() const {
    return asHandPokeLimiterVisual_;
  }
  operator isdk_HandPokeLimiterVisual*() const {
    return asHandPokeLimiterVisual_;
  }
  [[nodiscard]] isdk_IUpdate* getIUpdateHandle() const override {
    return asIUpdate_;
  }
  operator isdk_IUpdate*() const {
    return asIUpdate_;
  }

 protected:
  isdk_HandPokeLimiterVisual* asHandPokeLimiterVisual_;
  isdk_IUpdate* asIUpdate_;
};
using HandPokeLimiterVisualPtr = ObjectPtr<HandPokeLimiterVisual, isdk_HandPokeLimiterVisual>;

class InteractableStateEventQueue final {
 public:
  explicit InteractableStateEventQueue(isdk_InteractableStateEventQueue* handle = nullptr)
      : asInteractableStateEventQueue_(handle) {}
  static ObjectPtr<InteractableStateEventQueue, isdk_InteractableStateEventQueue> create(
      isdk_IInteractable* target) {
    isdk_InteractableStateEventQueue* retVal{};
    isdk_InteractableStateEventQueue_create(target, &retVal);
    return ObjectPtr<InteractableStateEventQueue, isdk_InteractableStateEventQueue>(
        retVal, &isdk_InteractableStateEventQueue_destroy);
  }
  static void destroy(isdk_InteractableStateEventQueue* ptr) {
    isdk_InteractableStateEventQueue_destroy(ptr);
  }
  unsigned char isEmpty() {
    unsigned char retVal{};
    isdk_InteractableStateEventQueue_isEmpty(getInteractableStateEventQueueHandle(), &retVal);
    return retVal;
  }

  unsigned long long getCount() {
    unsigned long long retVal{};
    isdk_InteractableStateEventQueue_getCount(getInteractableStateEventQueueHandle(), &retVal);
    return retVal;
  }

  isdk_InteractableStateChangeArgs pop() {
    isdk_InteractableStateChangeArgs retVal{};
    isdk_InteractableStateEventQueue_pop(getInteractableStateEventQueueHandle(), &retVal);
    return retVal;
  }

  void clear() {
    isdk_InteractableStateEventQueue_clear(getInteractableStateEventQueueHandle());
  }
  [[nodiscard]] isdk_InteractableStateEventQueue* getInteractableStateEventQueueHandle() const {
    return asInteractableStateEventQueue_;
  }
  operator isdk_InteractableStateEventQueue*() const {
    return asInteractableStateEventQueue_;
  }

 protected:
  isdk_InteractableStateEventQueue* asInteractableStateEventQueue_;
};
using InteractableStateEventQueuePtr =
    ObjectPtr<InteractableStateEventQueue, isdk_InteractableStateEventQueue>;

class InteractorPayload final : public IPayload {
 public:
  explicit InteractorPayload(isdk_InteractorPayload* handle = nullptr)
      : asInteractorPayload_(handle),
        asIPayload_(tryInitWithCast<isdk_InteractorPayload, isdk_IPayload>(
            handle,
            &isdk_InteractorPayload_castToIPayload)) {}
  static ObjectPtr<InteractorPayload, isdk_InteractorPayload> create(
      isdk_IInteractor* interactor,
      void* externalData) {
    isdk_InteractorPayload* retVal{};
    isdk_InteractorPayload_create(interactor, externalData, &retVal);
    return ObjectPtr<InteractorPayload, isdk_InteractorPayload>(
        retVal, &isdk_InteractorPayload_destroy);
  }
  static void destroy(isdk_InteractorPayload* ptr) {
    isdk_InteractorPayload_destroy(ptr);
  }
  isdk_IInteractor* getInteractor() {
    isdk_IInteractor* retVal{};
    isdk_InteractorPayload_getInteractor(getInteractorPayloadHandle(), &retVal);
    return retVal;
  }

  void setExternalData(void* externalData) {
    isdk_InteractorPayload_setExternalData(getInteractorPayloadHandle(), externalData);
  }

  void* getExternalData() {
    void* retVal{};
    isdk_InteractorPayload_getExternalData(getInteractorPayloadHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_InteractorPayload* getInteractorPayloadHandle() const {
    return asInteractorPayload_;
  }
  operator isdk_InteractorPayload*() const {
    return asInteractorPayload_;
  }
  [[nodiscard]] isdk_IPayload* getIPayloadHandle() const override {
    return asIPayload_;
  }
  operator isdk_IPayload*() const {
    return asIPayload_;
  }

 protected:
  isdk_InteractorPayload* asInteractorPayload_;
  isdk_IPayload* asIPayload_;
};
using InteractorPayloadPtr = ObjectPtr<InteractorPayload, isdk_InteractorPayload>;

class InteractorStateEventQueue final {
 public:
  explicit InteractorStateEventQueue(isdk_InteractorStateEventQueue* handle = nullptr)
      : asInteractorStateEventQueue_(handle) {}
  static ObjectPtr<InteractorStateEventQueue, isdk_InteractorStateEventQueue> create(
      isdk_IInteractor* target) {
    isdk_InteractorStateEventQueue* retVal{};
    isdk_InteractorStateEventQueue_create(target, &retVal);
    return ObjectPtr<InteractorStateEventQueue, isdk_InteractorStateEventQueue>(
        retVal, &isdk_InteractorStateEventQueue_destroy);
  }
  static void destroy(isdk_InteractorStateEventQueue* ptr) {
    isdk_InteractorStateEventQueue_destroy(ptr);
  }
  unsigned char isEmpty() {
    unsigned char retVal{};
    isdk_InteractorStateEventQueue_isEmpty(getInteractorStateEventQueueHandle(), &retVal);
    return retVal;
  }

  unsigned long long getCount() {
    unsigned long long retVal{};
    isdk_InteractorStateEventQueue_getCount(getInteractorStateEventQueueHandle(), &retVal);
    return retVal;
  }

  isdk_InteractorStateChangeArgs pop() {
    isdk_InteractorStateChangeArgs retVal{};
    isdk_InteractorStateEventQueue_pop(getInteractorStateEventQueueHandle(), &retVal);
    return retVal;
  }

  void clear() {
    isdk_InteractorStateEventQueue_clear(getInteractorStateEventQueueHandle());
  }

  void setEventTypeHint(const char* interactorTypeHint) {
    isdk_InteractorStateEventQueue_setEventTypeHint(
        getInteractorStateEventQueueHandle(), interactorTypeHint);
  }
  [[nodiscard]] isdk_InteractorStateEventQueue* getInteractorStateEventQueueHandle() const {
    return asInteractorStateEventQueue_;
  }
  operator isdk_InteractorStateEventQueue*() const {
    return asInteractorStateEventQueue_;
  }

 protected:
  isdk_InteractorStateEventQueue* asInteractorStateEventQueue_;
};
using InteractorStateEventQueuePtr =
    ObjectPtr<InteractorStateEventQueue, isdk_InteractorStateEventQueue>;

class OneEuroHandFilter final : public IHandDataModifier {
 public:
  explicit OneEuroHandFilter(isdk_OneEuroHandFilter* handle = nullptr)
      : asOneEuroHandFilter_(handle),
        asIHandDataSource_(tryInitWithCast<isdk_OneEuroHandFilter, isdk_IHandDataSource>(
            handle,
            &isdk_OneEuroHandFilter_castToIHandDataSource)),
        asIHandDataModifier_(tryInitWithCast<isdk_OneEuroHandFilter, isdk_IHandDataModifier>(
            handle,
            &isdk_OneEuroHandFilter_castToIHandDataModifier)) {}
  static ObjectPtr<OneEuroHandFilter, isdk_OneEuroHandFilter> create(
      isdk_IHandDataSource* fromDataSource,
      unsigned char recursiveUpdate) {
    isdk_OneEuroHandFilter* retVal{};
    isdk_OneEuroHandFilter_create(fromDataSource, recursiveUpdate, &retVal);
    return ObjectPtr<OneEuroHandFilter, isdk_OneEuroHandFilter>(
        retVal, &isdk_OneEuroHandFilter_destroy);
  }
  static void destroy(isdk_OneEuroHandFilter* ptr) {
    isdk_OneEuroHandFilter_destroy(ptr);
  }
  void setFromDataSource(isdk_IHandDataSource* fromDataSource) {
    isdk_OneEuroHandFilter_setFromDataSource(getOneEuroHandFilterHandle(), fromDataSource);
  }
  [[nodiscard]] isdk_OneEuroHandFilter* getOneEuroHandFilterHandle() const {
    return asOneEuroHandFilter_;
  }
  operator isdk_OneEuroHandFilter*() const {
    return asOneEuroHandFilter_;
  }
  [[nodiscard]] isdk_IHandDataSource* getIHandDataSourceHandle() const override {
    return asIHandDataSource_;
  }
  operator isdk_IHandDataSource*() const {
    return asIHandDataSource_;
  }
  [[nodiscard]] isdk_IHandDataModifier* getIHandDataModifierHandle() const override {
    return asIHandDataModifier_;
  }
  operator isdk_IHandDataModifier*() const {
    return asIHandDataModifier_;
  }

 protected:
  isdk_OneEuroHandFilter* asOneEuroHandFilter_;
  isdk_IHandDataSource* asIHandDataSource_;
  isdk_IHandDataModifier* asIHandDataModifier_;
};
using OneEuroHandFilterPtr = ObjectPtr<OneEuroHandFilter, isdk_OneEuroHandFilter>;

class PalmGrabRecognizer final : public IActiveState, public IDeltaTimeUpdate {
 public:
  explicit PalmGrabRecognizer(isdk_PalmGrabRecognizer* handle = nullptr)
      : asPalmGrabRecognizer_(handle),
        asIActiveState_(tryInitWithCast<isdk_PalmGrabRecognizer, isdk_IActiveState>(
            handle,
            &isdk_PalmGrabRecognizer_castToIActiveState)),
        asIDeltaTimeUpdate_(tryInitWithCast<isdk_PalmGrabRecognizer, isdk_IDeltaTimeUpdate>(
            handle,
            &isdk_PalmGrabRecognizer_castToIDeltaTimeUpdate)) {}
  static ObjectPtr<PalmGrabRecognizer, isdk_PalmGrabRecognizer> create(
      isdk_IHandPositionFrame* handPositionFrame) {
    isdk_PalmGrabRecognizer* retVal{};
    isdk_PalmGrabRecognizer_create(handPositionFrame, &retVal);
    return ObjectPtr<PalmGrabRecognizer, isdk_PalmGrabRecognizer>(
        retVal, &isdk_PalmGrabRecognizer_destroy);
  }
  static void destroy(isdk_PalmGrabRecognizer* ptr) {
    isdk_PalmGrabRecognizer_destroy(ptr);
  }
  void setInputTrackingConfidence(unsigned char isHighConfidence) {
    isdk_PalmGrabRecognizer_setInputTrackingConfidence(
        getPalmGrabRecognizerHandle(), isHighConfidence);
  }

  void update(float deltaTime) {
    isdk_PalmGrabRecognizer_update(getPalmGrabRecognizerHandle(), deltaTime);
  }

  unsigned char isActive() {
    unsigned char retVal{};
    isdk_PalmGrabRecognizer_isActive(getPalmGrabRecognizerHandle(), &retVal);
    return retVal;
  }

  void setFingerMinCurlValue(isdk_FingerType finger, float angle) {
    isdk_PalmGrabRecognizer_setFingerMinCurlValue(getPalmGrabRecognizerHandle(), finger, angle);
  }

  void setFingerMinCount(int count) {
    isdk_PalmGrabRecognizer_setFingerMinCount(getPalmGrabRecognizerHandle(), count);
  }
  [[nodiscard]] isdk_PalmGrabRecognizer* getPalmGrabRecognizerHandle() const {
    return asPalmGrabRecognizer_;
  }
  operator isdk_PalmGrabRecognizer*() const {
    return asPalmGrabRecognizer_;
  }
  [[nodiscard]] isdk_IActiveState* getIActiveStateHandle() const override {
    return asIActiveState_;
  }
  operator isdk_IActiveState*() const {
    return asIActiveState_;
  }
  [[nodiscard]] isdk_IDeltaTimeUpdate* getIDeltaTimeUpdateHandle() const override {
    return asIDeltaTimeUpdate_;
  }
  operator isdk_IDeltaTimeUpdate*() const {
    return asIDeltaTimeUpdate_;
  }

 protected:
  isdk_PalmGrabRecognizer* asPalmGrabRecognizer_;
  isdk_IActiveState* asIActiveState_;
  isdk_IDeltaTimeUpdate* asIDeltaTimeUpdate_;
};
using PalmGrabRecognizerPtr = ObjectPtr<PalmGrabRecognizer, isdk_PalmGrabRecognizer>;

class PointableAxisAlignedBox final : public ISurface {
 public:
  explicit PointableAxisAlignedBox(isdk_PointableAxisAlignedBox* handle = nullptr)
      : asPointableAxisAlignedBox_(handle),
        asIPose_(tryInitWithCast<isdk_PointableAxisAlignedBox, isdk_IPose>(
            handle,
            &isdk_PointableAxisAlignedBox_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_PointableAxisAlignedBox, isdk_IBounds>(
            handle,
            &isdk_PointableAxisAlignedBox_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_PointableAxisAlignedBox, isdk_ISurface>(
            handle,
            &isdk_PointableAxisAlignedBox_castToISurface)) {}
  static ObjectPtr<PointableAxisAlignedBox, isdk_PointableAxisAlignedBox> create() {
    isdk_PointableAxisAlignedBox* retVal{};
    isdk_PointableAxisAlignedBox_create(&retVal);
    return ObjectPtr<PointableAxisAlignedBox, isdk_PointableAxisAlignedBox>(
        retVal, &isdk_PointableAxisAlignedBox_destroy);
  }
  static ObjectPtr<PointableAxisAlignedBox, isdk_PointableAxisAlignedBox> createWithPose(
      const ovrpVector3f* centroid,
      const ovrpVector3f* size) {
    isdk_PointableAxisAlignedBox* retVal{};
    isdk_PointableAxisAlignedBox_createWithPose(centroid, size, &retVal);
    return ObjectPtr<PointableAxisAlignedBox, isdk_PointableAxisAlignedBox>(
        retVal, &isdk_PointableAxisAlignedBox_destroy);
  }
  static void destroy(isdk_PointableAxisAlignedBox* ptr) {
    isdk_PointableAxisAlignedBox_destroy(ptr);
  }
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableAxisAlignedBox_raycast(
        getPointableAxisAlignedBoxHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableAxisAlignedBox_closestSurfacePoint(
        getPointableAxisAlignedBoxHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  void setBounds(const ovrpVector3f* lowerBound, const ovrpVector3f* upperBound) {
    isdk_PointableAxisAlignedBox_setBounds(
        getPointableAxisAlignedBoxHandle(), lowerBound, upperBound);
  }

  void setCentroidAndSize(const ovrpVector3f* centroid, const ovrpVector3f* size) {
    isdk_PointableAxisAlignedBox_setCentroidAndSize(
        getPointableAxisAlignedBoxHandle(), centroid, size);
  }

  void setLowerBound(const ovrpVector3f* lowerBound) {
    isdk_PointableAxisAlignedBox_setLowerBound(getPointableAxisAlignedBoxHandle(), lowerBound);
  }

  void setUpperBound(const ovrpVector3f* upperBound) {
    isdk_PointableAxisAlignedBox_setUpperBound(getPointableAxisAlignedBoxHandle(), upperBound);
  }

  void setCentroid(const ovrpVector3f* centroid) {
    isdk_PointableAxisAlignedBox_setCentroid(getPointableAxisAlignedBoxHandle(), centroid);
  }

  void setSize(const ovrpVector3f* size) {
    isdk_PointableAxisAlignedBox_setSize(getPointableAxisAlignedBoxHandle(), size);
  }

  ovrpVector3f getLowerBound() {
    ovrpVector3f retVal{};
    isdk_PointableAxisAlignedBox_getLowerBound(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getUpperBound() {
    ovrpVector3f retVal{};
    isdk_PointableAxisAlignedBox_getUpperBound(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getCenter() {
    ovrpVector3f retVal{};
    isdk_PointableAxisAlignedBox_getCenter(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getSize() {
    ovrpVector3f retVal{};
    isdk_PointableAxisAlignedBox_getSize(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_PointableAxisAlignedBox_getPose(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_PointableAxisAlignedBox_getBounds(getPointableAxisAlignedBoxHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_PointableAxisAlignedBox* getPointableAxisAlignedBoxHandle() const {
    return asPointableAxisAlignedBox_;
  }
  operator isdk_PointableAxisAlignedBox*() const {
    return asPointableAxisAlignedBox_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }

 protected:
  isdk_PointableAxisAlignedBox* asPointableAxisAlignedBox_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
};
using PointableAxisAlignedBoxPtr = ObjectPtr<PointableAxisAlignedBox, isdk_PointableAxisAlignedBox>;

class PointableOrientedBox final : public ISurface {
 public:
  explicit PointableOrientedBox(isdk_PointableOrientedBox* handle = nullptr)
      : asPointableOrientedBox_(handle),
        asIPose_(tryInitWithCast<isdk_PointableOrientedBox, isdk_IPose>(
            handle,
            &isdk_PointableOrientedBox_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_PointableOrientedBox, isdk_IBounds>(
            handle,
            &isdk_PointableOrientedBox_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_PointableOrientedBox, isdk_ISurface>(
            handle,
            &isdk_PointableOrientedBox_castToISurface)) {}
  static ObjectPtr<PointableOrientedBox, isdk_PointableOrientedBox> create() {
    isdk_PointableOrientedBox* retVal{};
    isdk_PointableOrientedBox_create(&retVal);
    return ObjectPtr<PointableOrientedBox, isdk_PointableOrientedBox>(
        retVal, &isdk_PointableOrientedBox_destroy);
  }
  static ObjectPtr<PointableOrientedBox, isdk_PointableOrientedBox> createWithPose(
      const ovrpPosef* pose,
      const ovrpVector3f* size) {
    isdk_PointableOrientedBox* retVal{};
    isdk_PointableOrientedBox_createWithPose(pose, size, &retVal);
    return ObjectPtr<PointableOrientedBox, isdk_PointableOrientedBox>(
        retVal, &isdk_PointableOrientedBox_destroy);
  }
  static void destroy(isdk_PointableOrientedBox* ptr) {
    isdk_PointableOrientedBox_destroy(ptr);
  }
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableOrientedBox_raycast(
        getPointableOrientedBoxHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableOrientedBox_closestSurfacePoint(
        getPointableOrientedBoxHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  void setPoseAndSize(const ovrpPosef* centroid, const ovrpVector3f* size) {
    isdk_PointableOrientedBox_setPoseAndSize(getPointableOrientedBoxHandle(), centroid, size);
  }

  void setPose(const ovrpPosef* centroid) {
    isdk_PointableOrientedBox_setPose(getPointableOrientedBoxHandle(), centroid);
  }

  void setSize(const ovrpVector3f* size) {
    isdk_PointableOrientedBox_setSize(getPointableOrientedBoxHandle(), size);
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_PointableOrientedBox_getPose(getPointableOrientedBoxHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_PointableOrientedBox_getBounds(getPointableOrientedBoxHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_PointableOrientedBox* getPointableOrientedBoxHandle() const {
    return asPointableOrientedBox_;
  }
  operator isdk_PointableOrientedBox*() const {
    return asPointableOrientedBox_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }

 protected:
  isdk_PointableOrientedBox* asPointableOrientedBox_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
};
using PointableOrientedBoxPtr = ObjectPtr<PointableOrientedBox, isdk_PointableOrientedBox>;

class PointablePlane final : public ISurface {
 public:
  explicit PointablePlane(isdk_PointablePlane* handle = nullptr)
      : asPointablePlane_(handle),
        asIPose_(tryInitWithCast<isdk_PointablePlane, isdk_IPose>(
            handle,
            &isdk_PointablePlane_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_PointablePlane, isdk_IBounds>(
            handle,
            &isdk_PointablePlane_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_PointablePlane, isdk_ISurface>(
            handle,
            &isdk_PointablePlane_castToISurface)) {}
  static ObjectPtr<PointablePlane, isdk_PointablePlane> create() {
    isdk_PointablePlane* retVal{};
    isdk_PointablePlane_create(&retVal);
    return ObjectPtr<PointablePlane, isdk_PointablePlane>(retVal, &isdk_PointablePlane_destroy);
  }
  static ObjectPtr<PointablePlane, isdk_PointablePlane> createWithPose(
      const ovrpPosef* pose,
      const ovrpVector2f* size) {
    isdk_PointablePlane* retVal{};
    isdk_PointablePlane_createWithPose(pose, size, &retVal);
    return ObjectPtr<PointablePlane, isdk_PointablePlane>(retVal, &isdk_PointablePlane_destroy);
  }
  static void destroy(isdk_PointablePlane* ptr) {
    isdk_PointablePlane_destroy(ptr);
  }
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointablePlane_raycast(getPointablePlaneHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointablePlane_closestSurfacePoint(
        getPointablePlaneHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  void setPose(const ovrpPosef* pose) {
    isdk_PointablePlane_setPose(getPointablePlaneHandle(), pose);
  }

  void setSize(const ovrpVector2f* newSize) {
    isdk_PointablePlane_setSize(getPointablePlaneHandle(), newSize);
  }

  ovrpVector3f getNormal() {
    ovrpVector3f retVal{};
    isdk_PointablePlane_getNormal(getPointablePlaneHandle(), &retVal);
    return retVal;
  }

  ovrpVector2f getSize() {
    ovrpVector2f retVal{};
    isdk_PointablePlane_getSize(getPointablePlaneHandle(), &retVal);
    return retVal;
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_PointablePlane_getPose(getPointablePlaneHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_PointablePlane_getBounds(getPointablePlaneHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_PointablePlane* getPointablePlaneHandle() const {
    return asPointablePlane_;
  }
  operator isdk_PointablePlane*() const {
    return asPointablePlane_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }

 protected:
  isdk_PointablePlane* asPointablePlane_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
};
using PointablePlanePtr = ObjectPtr<PointablePlane, isdk_PointablePlane>;

class PointableTransformableUnitSquare final : public ISurface {
 public:
  explicit PointableTransformableUnitSquare(isdk_PointableTransformableUnitSquare* handle = nullptr)
      : asPointableTransformableUnitSquare_(handle),
        asIPose_(tryInitWithCast<isdk_PointableTransformableUnitSquare, isdk_IPose>(
            handle,
            &isdk_PointableTransformableUnitSquare_castToIPose)),
        asIBounds_(tryInitWithCast<isdk_PointableTransformableUnitSquare, isdk_IBounds>(
            handle,
            &isdk_PointableTransformableUnitSquare_castToIBounds)),
        asISurface_(tryInitWithCast<isdk_PointableTransformableUnitSquare, isdk_ISurface>(
            handle,
            &isdk_PointableTransformableUnitSquare_castToISurface)) {}
  static ObjectPtr<PointableTransformableUnitSquare, isdk_PointableTransformableUnitSquare> create(
      ovrpVector3f position,
      ovrpVector3f size) {
    isdk_PointableTransformableUnitSquare* retVal{};
    isdk_PointableTransformableUnitSquare_create(position, size, &retVal);
    return ObjectPtr<PointableTransformableUnitSquare, isdk_PointableTransformableUnitSquare>(
        retVal, &isdk_PointableTransformableUnitSquare_destroy);
  }
  static void destroy(isdk_PointableTransformableUnitSquare* ptr) {
    isdk_PointableTransformableUnitSquare_destroy(ptr);
  }
  unsigned char raycast(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableTransformableUnitSquare_raycast(
        getPointableTransformableUnitSquareHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  closestSurfacePoint(const ovrpVector3f* point, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_PointableTransformableUnitSquare_closestSurfacePoint(
        getPointableTransformableUnitSquareHandle(), point, outHit, maxDistance, &retVal);
    return retVal;
  }

  ovrpPosef getPose() {
    ovrpPosef retVal{};
    isdk_PointableTransformableUnitSquare_getPose(
        getPointableTransformableUnitSquareHandle(), &retVal);
    return retVal;
  }

  isdk_AxisAlignedBox getBounds() {
    isdk_AxisAlignedBox retVal{};
    isdk_PointableTransformableUnitSquare_getBounds(
        getPointableTransformableUnitSquareHandle(), &retVal);
    return retVal;
  }

  void setTransform(const ovrpPosef* centroid, const ovrpVector3f* scale) {
    isdk_PointableTransformableUnitSquare_setTransform(
        getPointableTransformableUnitSquareHandle(), centroid, scale);
  }
  [[nodiscard]] isdk_PointableTransformableUnitSquare* getPointableTransformableUnitSquareHandle()
      const {
    return asPointableTransformableUnitSquare_;
  }
  operator isdk_PointableTransformableUnitSquare*() const {
    return asPointableTransformableUnitSquare_;
  }
  [[nodiscard]] isdk_IPose* getIPoseHandle() const override {
    return asIPose_;
  }
  operator isdk_IPose*() const {
    return asIPose_;
  }
  [[nodiscard]] isdk_IBounds* getIBoundsHandle() const override {
    return asIBounds_;
  }
  operator isdk_IBounds*() const {
    return asIBounds_;
  }
  [[nodiscard]] isdk_ISurface* getISurfaceHandle() const override {
    return asISurface_;
  }
  operator isdk_ISurface*() const {
    return asISurface_;
  }

 protected:
  isdk_PointableTransformableUnitSquare* asPointableTransformableUnitSquare_;
  isdk_IPose* asIPose_;
  isdk_IBounds* asIBounds_;
  isdk_ISurface* asISurface_;
};
using PointableTransformableUnitSquarePtr =
    ObjectPtr<PointableTransformableUnitSquare, isdk_PointableTransformableUnitSquare>;

class PointerEventQueue final {
 public:
  explicit PointerEventQueue(isdk_PointerEventQueue* handle = nullptr)
      : asPointerEventQueue_(handle) {}
  static ObjectPtr<PointerEventQueue, isdk_PointerEventQueue> create(isdk_IPointable* pointable) {
    isdk_PointerEventQueue* retVal{};
    isdk_PointerEventQueue_create(pointable, &retVal);
    return ObjectPtr<PointerEventQueue, isdk_PointerEventQueue>(
        retVal, &isdk_PointerEventQueue_destroy);
  }
  static void destroy(isdk_PointerEventQueue* ptr) {
    isdk_PointerEventQueue_destroy(ptr);
  }
  unsigned char isEmpty() {
    unsigned char retVal{};
    isdk_PointerEventQueue_isEmpty(getPointerEventQueueHandle(), &retVal);
    return retVal;
  }

  unsigned long long getCount() {
    unsigned long long retVal{};
    isdk_PointerEventQueue_getCount(getPointerEventQueueHandle(), &retVal);
    return retVal;
  }

  isdk_PointerEvent pop() {
    isdk_PointerEvent retVal{};
    isdk_PointerEventQueue_pop(getPointerEventQueueHandle(), &retVal);
    return retVal;
  }

  void clear() {
    isdk_PointerEventQueue_clear(getPointerEventQueueHandle());
  }
  [[nodiscard]] isdk_PointerEventQueue* getPointerEventQueueHandle() const {
    return asPointerEventQueue_;
  }
  operator isdk_PointerEventQueue*() const {
    return asPointerEventQueue_;
  }

 protected:
  isdk_PointerEventQueue* asPointerEventQueue_;
};
using PointerEventQueuePtr = ObjectPtr<PointerEventQueue, isdk_PointerEventQueue>;

class PokeButtonVisual final {
 public:
  explicit PokeButtonVisual(isdk_PokeButtonVisual* handle = nullptr)
      : asPokeButtonVisual_(handle) {}
  static ObjectPtr<PokeButtonVisual, isdk_PokeButtonVisual>
  create(isdk_PokeInteractable* pokeInteractable, ovrpPosef basePose, float maxExtent) {
    isdk_PokeButtonVisual* retVal{};
    isdk_PokeButtonVisual_create(pokeInteractable, basePose, maxExtent, &retVal);
    return ObjectPtr<PokeButtonVisual, isdk_PokeButtonVisual>(
        retVal, &isdk_PokeButtonVisual_destroy);
  }
  static void destroy(isdk_PokeButtonVisual* ptr) {
    isdk_PokeButtonVisual_destroy(ptr);
  }
  ovrpPosef getBasePose() {
    ovrpPosef retVal{};
    isdk_PokeButtonVisual_getBasePose(getPokeButtonVisualHandle(), &retVal);
    return retVal;
  }

  float getMaxExtent() {
    float retVal{};
    isdk_PokeButtonVisual_getMaxExtent(getPokeButtonVisualHandle(), &retVal);
    return retVal;
  }

  float getCurrentExtent() {
    float retVal{};
    isdk_PokeButtonVisual_getCurrentExtent(getPokeButtonVisualHandle(), &retVal);
    return retVal;
  }

  ovrpPosef getCurrentPose() {
    ovrpPosef retVal{};
    isdk_PokeButtonVisual_getCurrentPose(getPokeButtonVisualHandle(), &retVal);
    return retVal;
  }

  void setPokeInteractable(
      isdk_PokeInteractable* pokeInteractable,
      ovrpPosef basePose,
      float extentDistance) {
    isdk_PokeButtonVisual_setPokeInteractable(
        getPokeButtonVisualHandle(), pokeInteractable, basePose, extentDistance);
  }

  void updateBasePose(const ovrpPosef* basePose, float extentDistance) {
    isdk_PokeButtonVisual_updateBasePose(getPokeButtonVisualHandle(), basePose, extentDistance);
  }

  static float calculateExtentFromState(isdk_PokeInteractable* interactable, float maxExtent) {
    float retVal{};
    isdk_PokeButtonVisual_calculateExtentFromState(interactable, maxExtent, &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_PokeButtonVisual* getPokeButtonVisualHandle() const {
    return asPokeButtonVisual_;
  }
  operator isdk_PokeButtonVisual*() const {
    return asPokeButtonVisual_;
  }

 protected:
  isdk_PokeButtonVisual* asPokeButtonVisual_;
};
using PokeButtonVisualPtr = ObjectPtr<PokeButtonVisual, isdk_PokeButtonVisual>;

class PokeInteractable final : public IPointable, public IInteractable, public IEnable {
 public:
  explicit PokeInteractable(isdk_PokeInteractable* handle = nullptr)
      : asPokeInteractable_(handle),
        asIPointable_(tryInitWithCast<isdk_PokeInteractable, isdk_IPointable>(
            handle,
            &isdk_PokeInteractable_castToIPointable)),
        asIInteractable_(tryInitWithCast<isdk_PokeInteractable, isdk_IInteractable>(
            handle,
            &isdk_PokeInteractable_castToIInteractable)),
        asIEnable_(tryInitWithCast<isdk_PokeInteractable, isdk_IEnable>(
            handle,
            &isdk_PokeInteractable_castToIEnable)) {}
  static ObjectPtr<PokeInteractable, isdk_PokeInteractable> create(
      const isdk_PokeInteractable_Config* config,
      isdk_ISurfacePatch* surfacePatch,
      isdk_IPointableElement* pointableElement) {
    isdk_PokeInteractable* retVal{};
    isdk_PokeInteractable_create(config, surfacePatch, pointableElement, &retVal);
    return ObjectPtr<PokeInteractable, isdk_PokeInteractable>(
        retVal, &isdk_PokeInteractable_destroy);
  }
  static void destroy(isdk_PokeInteractable* ptr) {
    isdk_PokeInteractable_destroy(ptr);
  }
  unsigned char closestSurfacePatchHit(const ovrpVector3f* point, isdk_SurfaceHit* hit) {
    unsigned char retVal{};
    isdk_PokeInteractable_closestSurfacePatchHit(getPokeInteractableHandle(), point, hit, &retVal);
    return retVal;
  }

  unsigned char closestBackingSurfaceHit(const ovrpVector3f* point, isdk_SurfaceHit* hit) {
    unsigned char retVal{};
    isdk_PokeInteractable_closestBackingSurfaceHit(
        getPokeInteractableHandle(), point, hit, &retVal);
    return retVal;
  }

  isdk_ISurfacePatch* getSurfacePatch() {
    isdk_ISurfacePatch* retVal{};
    isdk_PokeInteractable_getSurfacePatch(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getEnterHoverNormal() {
    float retVal{};
    isdk_PokeInteractable_getEnterHoverNormal(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getEnterHoverTangent() {
    float retVal{};
    isdk_PokeInteractable_getEnterHoverTangent(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getExitHoverNormal() {
    float retVal{};
    isdk_PokeInteractable_getExitHoverNormal(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getExitHoverTangent() {
    float retVal{};
    isdk_PokeInteractable_getExitHoverTangent(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getCancelSelectNormal() {
    float retVal{};
    isdk_PokeInteractable_getCancelSelectNormal(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getCancelSelectTangent() {
    float retVal{};
    isdk_PokeInteractable_getCancelSelectTangent(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  void setSurfacePatch(isdk_ISurfacePatch* surfacePatch) {
    isdk_PokeInteractable_setSurfacePatch(getPokeInteractableHandle(), surfacePatch);
  }

  isdk_PokeInteractable_Config getConfig() {
    isdk_PokeInteractable_Config retVal{};
    isdk_PokeInteractable_getConfig(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  void setConfig(const isdk_PokeInteractable_Config* config) {
    isdk_PokeInteractable_setConfig(getPokeInteractableHandle(), config);
  }

  isdk_PokeInteractable_PositionPinningConfig getPositionPinning() {
    isdk_PokeInteractable_PositionPinningConfig retVal{};
    isdk_PokeInteractable_getPositionPinning(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  void setPositionPinning(
      const isdk_PokeInteractable_PositionPinningConfig* positionPinningConfig) {
    isdk_PokeInteractable_setPositionPinning(getPokeInteractableHandle(), positionPinningConfig);
  }

  isdk_PokeInteractable_DragThresholdingConfig getDragThresholds() {
    isdk_PokeInteractable_DragThresholdingConfig retVal{};
    isdk_PokeInteractable_getDragThresholds(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  isdk_PokeInteractable_MinThresholdsConfig getMinThresholds() {
    isdk_PokeInteractable_MinThresholdsConfig retVal{};
    isdk_PokeInteractable_getMinThresholds(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  float getCloseDistanceThreshold() {
    float retVal{};
    isdk_PokeInteractable_getCloseDistanceThreshold(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  void setCloseDistanceThreshold(float paramVar0) {
    isdk_PokeInteractable_setCloseDistanceThreshold(getPokeInteractableHandle(), paramVar0);
  }

  int getTiebreakerScore() {
    int retVal{};
    isdk_PokeInteractable_getTiebreakerScore(getPokeInteractableHandle(), &retVal);
    return retVal;
  }

  void setTiebreakerScore(int score) {
    isdk_PokeInteractable_setTiebreakerScore(getPokeInteractableHandle(), score);
  }
  [[nodiscard]] isdk_PokeInteractable* getPokeInteractableHandle() const {
    return asPokeInteractable_;
  }
  operator isdk_PokeInteractable*() const {
    return asPokeInteractable_;
  }
  [[nodiscard]] isdk_IPointable* getIPointableHandle() const override {
    return asIPointable_;
  }
  operator isdk_IPointable*() const {
    return asIPointable_;
  }
  [[nodiscard]] isdk_IInteractable* getIInteractableHandle() const override {
    return asIInteractable_;
  }
  operator isdk_IInteractable*() const {
    return asIInteractable_;
  }
  [[nodiscard]] isdk_IEnable* getIEnableHandle() const override {
    return asIEnable_;
  }
  operator isdk_IEnable*() const {
    return asIEnable_;
  }

 protected:
  isdk_PokeInteractable* asPokeInteractable_;
  isdk_IPointable* asIPointable_;
  isdk_IInteractable* asIInteractable_;
  isdk_IEnable* asIEnable_;
};
using PokeInteractablePtr = ObjectPtr<PokeInteractable, isdk_PokeInteractable>;

class PokeInteractor final : public IInteractor, public IEnable, public IHasSelector {
 public:
  explicit PokeInteractor(isdk_PokeInteractor* handle = nullptr)
      : asPokeInteractor_(handle),
        asIInteractorView_(tryInitWithCast<isdk_PokeInteractor, isdk_IInteractorView>(
            handle,
            &isdk_PokeInteractor_castToIInteractorView)),
        asIUpdate_(tryInitWithCast<isdk_PokeInteractor, isdk_IUpdate>(
            handle,
            &isdk_PokeInteractor_castToIUpdate)),
        asIUpdateDriver_(tryInitWithCast<isdk_PokeInteractor, isdk_IUpdateDriver>(
            handle,
            &isdk_PokeInteractor_castToIUpdateDriver)),
        asIInteractor_(tryInitWithCast<isdk_PokeInteractor, isdk_IInteractor>(
            handle,
            &isdk_PokeInteractor_castToIInteractor)),
        asIEnable_(tryInitWithCast<isdk_PokeInteractor, isdk_IEnable>(
            handle,
            &isdk_PokeInteractor_castToIEnable)),
        asIHasSelector_(tryInitWithCast<isdk_PokeInteractor, isdk_IHasSelector>(
            handle,
            &isdk_PokeInteractor_castToIHasSelector)) {}
  static ObjectPtr<PokeInteractor, isdk_PokeInteractor> createWithOptions(
      const isdk_PokeInteractor_Config* config,
      isdk_IActiveState* activeState,
      isdk_IPayload* payload,
      isdk_IPosition* positionProvider,
      isdk_ITimeProvider* timeProvider) {
    isdk_PokeInteractor* retVal{};
    isdk_PokeInteractor_createWithOptions(
        config, activeState, payload, positionProvider, timeProvider, &retVal);
    return ObjectPtr<PokeInteractor, isdk_PokeInteractor>(retVal, &isdk_PokeInteractor_destroy);
  }
  static ObjectPtr<PokeInteractor, isdk_PokeInteractor> create() {
    isdk_PokeInteractor* retVal{};
    isdk_PokeInteractor_create(&retVal);
    return ObjectPtr<PokeInteractor, isdk_PokeInteractor>(retVal, &isdk_PokeInteractor_destroy);
  }
  static void destroy(isdk_PokeInteractor* ptr) {
    isdk_PokeInteractor_destroy(ptr);
  }
  void setConfig(const isdk_PokeInteractor_Config* config) {
    isdk_PokeInteractor_setConfig(getPokeInteractorHandle(), config);
  }

  void setPointTransform(const ovrpPosef* pointTransform) {
    isdk_PokeInteractor_setPointTransform(getPokeInteractorHandle(), pointTransform);
  }

  void setPointPosition(const ovrpVector3f* pointPosition) {
    isdk_PokeInteractor_setPointPosition(getPokeInteractorHandle(), pointPosition);
  }

  ovrpVector3f getClosestPoint() {
    ovrpVector3f retVal{};
    isdk_PokeInteractor_getClosestPoint(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getTouchPoint() {
    ovrpVector3f retVal{};
    isdk_PokeInteractor_getTouchPoint(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getTouchNormal() {
    ovrpVector3f retVal{};
    isdk_PokeInteractor_getTouchNormal(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  float getRadius() {
    float retVal{};
    isdk_PokeInteractor_getRadius(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getOrigin() {
    ovrpVector3f retVal{};
    isdk_PokeInteractor_getOrigin(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  unsigned char isPassedSurface() {
    unsigned char retVal{};
    isdk_PokeInteractor_isPassedSurface(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  unsigned char isDebouncing() {
    unsigned char retVal{};
    isdk_PokeInteractor_isDebouncing(getPokeInteractorHandle(), &retVal);
    return retVal;
  }

  static float computeDistanceAbove(
      isdk_PokeInteractable* interactable,
      const ovrpVector3f* point,
      float radius) {
    float retVal{};
    isdk_PokeInteractor_computeDistanceAbove(interactable, point, radius, &retVal);
    return retVal;
  }

  static float
  computeDepth(isdk_PokeInteractable* interactable, const ovrpVector3f* point, float radius) {
    float retVal{};
    isdk_PokeInteractor_computeDepth(interactable, point, radius, &retVal);
    return retVal;
  }

  static float computeDistanceFrom(
      isdk_PokeInteractable* interactable,
      const ovrpVector3f* point,
      float radius) {
    float retVal{};
    isdk_PokeInteractor_computeDistanceFrom(interactable, point, radius, &retVal);
    return retVal;
  }

  static float computeTangentDistance(
      isdk_PokeInteractable* interactable,
      const ovrpVector3f* point,
      float radius) {
    float retVal{};
    isdk_PokeInteractor_computeTangentDistance(interactable, point, radius, &retVal);
    return retVal;
  }

  static float computeLateralDistanceToClosestPoint(
      const ovrpVector3f* point,
      float radius,
      const ovrpVector3f* closestProximityPoint,
      const ovrpVector3f* closestSurfaceNormal) {
    float retVal{};
    isdk_PokeInteractor_computeLateralDistanceToClosestPoint(
        point, radius, closestProximityPoint, closestSurfaceNormal, &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_PokeInteractor* getPokeInteractorHandle() const {
    return asPokeInteractor_;
  }
  operator isdk_PokeInteractor*() const {
    return asPokeInteractor_;
  }
  [[nodiscard]] isdk_IInteractorView* getIInteractorViewHandle() const override {
    return asIInteractorView_;
  }
  operator isdk_IInteractorView*() const {
    return asIInteractorView_;
  }
  [[nodiscard]] isdk_IUpdate* getIUpdateHandle() const override {
    return asIUpdate_;
  }
  operator isdk_IUpdate*() const {
    return asIUpdate_;
  }
  [[nodiscard]] isdk_IUpdateDriver* getIUpdateDriverHandle() const override {
    return asIUpdateDriver_;
  }
  operator isdk_IUpdateDriver*() const {
    return asIUpdateDriver_;
  }
  [[nodiscard]] isdk_IInteractor* getIInteractorHandle() const override {
    return asIInteractor_;
  }
  operator isdk_IInteractor*() const {
    return asIInteractor_;
  }
  [[nodiscard]] isdk_IEnable* getIEnableHandle() const override {
    return asIEnable_;
  }
  operator isdk_IEnable*() const {
    return asIEnable_;
  }
  [[nodiscard]] isdk_IHasSelector* getIHasSelectorHandle() const override {
    return asIHasSelector_;
  }
  operator isdk_IHasSelector*() const {
    return asIHasSelector_;
  }

 protected:
  isdk_PokeInteractor* asPokeInteractor_;
  isdk_IInteractorView* asIInteractorView_;
  isdk_IUpdate* asIUpdate_;
  isdk_IUpdateDriver* asIUpdateDriver_;
  isdk_IInteractor* asIInteractor_;
  isdk_IEnable* asIEnable_;
  isdk_IHasSelector* asIHasSelector_;
};
using PokeInteractorPtr = ObjectPtr<PokeInteractor, isdk_PokeInteractor>;

class RayInteractable final : public IPointable, public IInteractable, public IEnable {
 public:
  explicit RayInteractable(isdk_RayInteractable* handle = nullptr)
      : asRayInteractable_(handle),
        asIPointable_(tryInitWithCast<isdk_RayInteractable, isdk_IPointable>(
            handle,
            &isdk_RayInteractable_castToIPointable)),
        asIInteractable_(tryInitWithCast<isdk_RayInteractable, isdk_IInteractable>(
            handle,
            &isdk_RayInteractable_castToIInteractable)),
        asIEnable_(tryInitWithCast<isdk_RayInteractable, isdk_IEnable>(
            handle,
            &isdk_RayInteractable_castToIEnable)) {}
  static ObjectPtr<RayInteractable, isdk_RayInteractable> create(
      isdk_ISurface* surface,
      isdk_IPointableElement* pointableElement,
      isdk_ISurface* selectSurface,
      isdk_IMovementProvider* movementProvider,
      isdk_ISurface* magnetismSurface) {
    isdk_RayInteractable* retVal{};
    isdk_RayInteractable_create(
        surface, pointableElement, selectSurface, movementProvider, magnetismSurface, &retVal);
    return ObjectPtr<RayInteractable, isdk_RayInteractable>(retVal, &isdk_RayInteractable_destroy);
  }
  static void destroy(isdk_RayInteractable* ptr) {
    isdk_RayInteractable_destroy(ptr);
  }
  isdk_ISurface* getSurface() {
    isdk_ISurface* retVal{};
    isdk_RayInteractable_getSurface(getRayInteractableHandle(), &retVal);
    return retVal;
  }

  unsigned char
  raycastSelectSurface(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_RayInteractable_raycastSelectSurface(
        getRayInteractableHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  unsigned char
  raycastMagnetismSurface(const isdk_Ray* ray, isdk_SurfaceHit* outHit, float maxDistance) {
    unsigned char retVal{};
    isdk_RayInteractable_raycastMagnetismSurface(
        getRayInteractableHandle(), ray, outHit, maxDistance, &retVal);
    return retVal;
  }

  void setSurface(isdk_ISurface* surface) {
    isdk_RayInteractable_setSurface(getRayInteractableHandle(), surface);
  }

  void setSelectSurface(isdk_ISurface* surface) {
    isdk_RayInteractable_setSelectSurface(getRayInteractableHandle(), surface);
  }

  int getTiebreakerScore() {
    int retVal{};
    isdk_RayInteractable_getTiebreakerScore(getRayInteractableHandle(), &retVal);
    return retVal;
  }

  void setTiebreakerScore(int score) {
    isdk_RayInteractable_setTiebreakerScore(getRayInteractableHandle(), score);
  }
  [[nodiscard]] isdk_RayInteractable* getRayInteractableHandle() const {
    return asRayInteractable_;
  }
  operator isdk_RayInteractable*() const {
    return asRayInteractable_;
  }
  [[nodiscard]] isdk_IPointable* getIPointableHandle() const override {
    return asIPointable_;
  }
  operator isdk_IPointable*() const {
    return asIPointable_;
  }
  [[nodiscard]] isdk_IInteractable* getIInteractableHandle() const override {
    return asIInteractable_;
  }
  operator isdk_IInteractable*() const {
    return asIInteractable_;
  }
  [[nodiscard]] isdk_IEnable* getIEnableHandle() const override {
    return asIEnable_;
  }
  operator isdk_IEnable*() const {
    return asIEnable_;
  }

 protected:
  isdk_RayInteractable* asRayInteractable_;
  isdk_IPointable* asIPointable_;
  isdk_IInteractable* asIInteractable_;
  isdk_IEnable* asIEnable_;
};
using RayInteractablePtr = ObjectPtr<RayInteractable, isdk_RayInteractable>;

class RayInteractor final : public IInteractor, public IEnable, public IHasSelector {
 public:
  explicit RayInteractor(isdk_RayInteractor* handle = nullptr)
      : asRayInteractor_(handle),
        asIInteractorView_(tryInitWithCast<isdk_RayInteractor, isdk_IInteractorView>(
            handle,
            &isdk_RayInteractor_castToIInteractorView)),
        asIUpdate_(tryInitWithCast<isdk_RayInteractor, isdk_IUpdate>(
            handle,
            &isdk_RayInteractor_castToIUpdate)),
        asIUpdateDriver_(tryInitWithCast<isdk_RayInteractor, isdk_IUpdateDriver>(
            handle,
            &isdk_RayInteractor_castToIUpdateDriver)),
        asIInteractor_(tryInitWithCast<isdk_RayInteractor, isdk_IInteractor>(
            handle,
            &isdk_RayInteractor_castToIInteractor)),
        asIEnable_(tryInitWithCast<isdk_RayInteractor, isdk_IEnable>(
            handle,
            &isdk_RayInteractor_castToIEnable)),
        asIHasSelector_(tryInitWithCast<isdk_RayInteractor, isdk_IHasSelector>(
            handle,
            &isdk_RayInteractor_castToIHasSelector)) {}
  static ObjectPtr<RayInteractor, isdk_RayInteractor> create(
      const isdk_RayInteractor_Config* config,
      isdk_IPose* poseProvider,
      isdk_ISelector* selector,
      isdk_IActiveState* activeState,
      isdk_IPayload* payload) {
    isdk_RayInteractor* retVal{};
    isdk_RayInteractor_create(config, poseProvider, selector, activeState, payload, &retVal);
    return ObjectPtr<RayInteractor, isdk_RayInteractor>(retVal, &isdk_RayInteractor_destroy);
  }
  static void destroy(isdk_RayInteractor* ptr) {
    isdk_RayInteractor_destroy(ptr);
  }
  ovrpVector3f getOrigin() {
    ovrpVector3f retVal{};
    isdk_RayInteractor_getOrigin(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpQuatf getRotation() {
    ovrpQuatf retVal{};
    isdk_RayInteractor_getRotation(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  float getMaxRayLength() {
    float retVal{};
    isdk_RayInteractor_getMaxRayLength(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getForward() {
    ovrpVector3f retVal{};
    isdk_RayInteractor_getForward(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getUp() {
    ovrpVector3f retVal{};
    isdk_RayInteractor_getUp(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  ovrpVector3f getEnd() {
    ovrpVector3f retVal{};
    isdk_RayInteractor_getEnd(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  isdk_OptionalSurfaceHit getCollisionInfo() {
    isdk_OptionalSurfaceHit retVal{};
    isdk_RayInteractor_getCollisionInfo(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  void setOrigin(ovrpVector3f origin) {
    isdk_RayInteractor_setOrigin(getRayInteractorHandle(), origin);
  }

  void setRotation(ovrpQuatf rotation) {
    isdk_RayInteractor_setRotation(getRayInteractorHandle(), rotation);
  }

  void setMaxRayLength(float maxRayLength) {
    isdk_RayInteractor_setMaxRayLength(getRayInteractorHandle(), maxRayLength);
  }

  void setRayOrigin(const ovrpPosef* rayOrigin) {
    isdk_RayInteractor_setRayOrigin(getRayInteractorHandle(), rayOrigin);
  }

  void setLocalForwardVector(const ovrpVector3f vec) {
    isdk_RayInteractor_setLocalForwardVector(getRayInteractorHandle(), vec);
  }

  void setLocalUpVector(const ovrpVector3f vec) {
    isdk_RayInteractor_setLocalUpVector(getRayInteractorHandle(), vec);
  }

  isdk_RayInteractor_Config getConfig() {
    isdk_RayInteractor_Config retVal{};
    isdk_RayInteractor_getConfig(getRayInteractorHandle(), &retVal);
    return retVal;
  }

  void setConfig(const isdk_RayInteractor_Config* config) {
    isdk_RayInteractor_setConfig(getRayInteractorHandle(), config);
  }

  isdk_ICandidateProperty* getCandidateProperties() {
    isdk_ICandidateProperty* retVal{};
    isdk_RayInteractor_getCandidateProperties(getRayInteractorHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_RayInteractor* getRayInteractorHandle() const {
    return asRayInteractor_;
  }
  operator isdk_RayInteractor*() const {
    return asRayInteractor_;
  }
  [[nodiscard]] isdk_IInteractorView* getIInteractorViewHandle() const override {
    return asIInteractorView_;
  }
  operator isdk_IInteractorView*() const {
    return asIInteractorView_;
  }
  [[nodiscard]] isdk_IUpdate* getIUpdateHandle() const override {
    return asIUpdate_;
  }
  operator isdk_IUpdate*() const {
    return asIUpdate_;
  }
  [[nodiscard]] isdk_IUpdateDriver* getIUpdateDriverHandle() const override {
    return asIUpdateDriver_;
  }
  operator isdk_IUpdateDriver*() const {
    return asIUpdateDriver_;
  }
  [[nodiscard]] isdk_IInteractor* getIInteractorHandle() const override {
    return asIInteractor_;
  }
  operator isdk_IInteractor*() const {
    return asIInteractor_;
  }
  [[nodiscard]] isdk_IEnable* getIEnableHandle() const override {
    return asIEnable_;
  }
  operator isdk_IEnable*() const {
    return asIEnable_;
  }
  [[nodiscard]] isdk_IHasSelector* getIHasSelectorHandle() const override {
    return asIHasSelector_;
  }
  operator isdk_IHasSelector*() const {
    return asIHasSelector_;
  }

 protected:
  isdk_RayInteractor* asRayInteractor_;
  isdk_IInteractorView* asIInteractorView_;
  isdk_IUpdate* asIUpdate_;
  isdk_IUpdateDriver* asIUpdateDriver_;
  isdk_IInteractor* asIInteractor_;
  isdk_IEnable* asIEnable_;
  isdk_IHasSelector* asIHasSelector_;
};
using RayInteractorPtr = ObjectPtr<RayInteractor, isdk_RayInteractor>;

class ScaledTimeProvider final : public ITimeProvider {
 public:
  explicit ScaledTimeProvider(isdk_ScaledTimeProvider* handle = nullptr)
      : asScaledTimeProvider_(handle),
        asITimeProvider_(tryInitWithCast<isdk_ScaledTimeProvider, isdk_ITimeProvider>(
            handle,
            &isdk_ScaledTimeProvider_castToITimeProvider)) {}
  static ObjectPtr<ScaledTimeProvider, isdk_ScaledTimeProvider> create() {
    isdk_ScaledTimeProvider* retVal{};
    isdk_ScaledTimeProvider_create(&retVal);
    return ObjectPtr<ScaledTimeProvider, isdk_ScaledTimeProvider>(
        retVal, &isdk_ScaledTimeProvider_destroy);
  }
  static void destroy(isdk_ScaledTimeProvider* ptr) {
    isdk_ScaledTimeProvider_destroy(ptr);
  }
  void setScaledTimeSeconds(double scaledTimeSeconds) {
    isdk_ScaledTimeProvider_setScaledTimeSeconds(getScaledTimeProviderHandle(), scaledTimeSeconds);
  }

  double getScaledTimeSeconds() {
    double retVal{};
    isdk_ScaledTimeProvider_getScaledTimeSeconds(getScaledTimeProviderHandle(), &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_ScaledTimeProvider* getScaledTimeProviderHandle() const {
    return asScaledTimeProvider_;
  }
  operator isdk_ScaledTimeProvider*() const {
    return asScaledTimeProvider_;
  }
  [[nodiscard]] isdk_ITimeProvider* getITimeProviderHandle() const override {
    return asITimeProvider_;
  }
  operator isdk_ITimeProvider*() const {
    return asITimeProvider_;
  }

 protected:
  isdk_ScaledTimeProvider* asScaledTimeProvider_;
  isdk_ITimeProvider* asITimeProvider_;
};
using ScaledTimeProviderPtr = ObjectPtr<ScaledTimeProvider, isdk_ScaledTimeProvider>;

class Selector final : public ISelector {
 public:
  explicit Selector(isdk_Selector* handle = nullptr)
      : asSelector_(handle),
        asISelector_(tryInitWithCast<isdk_Selector, isdk_ISelector>(
            handle,
            &isdk_Selector_castToISelector)) {}
  static ObjectPtr<Selector, isdk_Selector> create() {
    isdk_Selector* retVal{};
    isdk_Selector_create(&retVal);
    return ObjectPtr<Selector, isdk_Selector>(retVal, &isdk_Selector_destroy);
  }
  static void destroy(isdk_Selector* ptr) {
    isdk_Selector_destroy(ptr);
  }
  void select() {
    isdk_Selector_select(getSelectorHandle());
  }

  void unselect() {
    isdk_Selector_unselect(getSelectorHandle());
  }
  [[nodiscard]] isdk_Selector* getSelectorHandle() const {
    return asSelector_;
  }
  operator isdk_Selector*() const {
    return asSelector_;
  }
  [[nodiscard]] isdk_ISelector* getISelectorHandle() const override {
    return asISelector_;
  }
  operator isdk_ISelector*() const {
    return asISelector_;
  }

 protected:
  isdk_Selector* asSelector_;
  isdk_ISelector* asISelector_;
};
using SelectorPtr = ObjectPtr<Selector, isdk_Selector>;

class SyntheticHand final : public IHandDataModifier {
 public:
  explicit SyntheticHand(isdk_SyntheticHand* handle = nullptr)
      : asSyntheticHand_(handle),
        asIHandDataSource_(tryInitWithCast<isdk_SyntheticHand, isdk_IHandDataSource>(
            handle,
            &isdk_SyntheticHand_castToIHandDataSource)),
        asIHandDataModifier_(tryInitWithCast<isdk_SyntheticHand, isdk_IHandDataModifier>(
            handle,
            &isdk_SyntheticHand_castToIHandDataModifier)) {}
  static ObjectPtr<SyntheticHand, isdk_SyntheticHand> create(
      isdk_IHandDataSource* fromDataSource,
      unsigned char recursiveUpdate,
      isdk_SyntheticHand_Config config) {
    isdk_SyntheticHand* retVal{};
    isdk_SyntheticHand_create(fromDataSource, recursiveUpdate, config, &retVal);
    return ObjectPtr<SyntheticHand, isdk_SyntheticHand>(retVal, &isdk_SyntheticHand_destroy);
  }
  static void destroy(isdk_SyntheticHand* ptr) {
    isdk_SyntheticHand_destroy(ptr);
  }
  static unsigned char isFlagSet(
      isdk_SyntheticHand_WristLockMode statusFlags,
      isdk_SyntheticHand_WristLockMode flagToLookFor) {
    unsigned char retVal{};
    isdk_SyntheticHand_isFlagSet(statusFlags, flagToLookFor, &retVal);
    return retVal;
  }

  void lockWristPose(
      const ovrpPosef* wristPose,
      const float overrideFactor,
      isdk_SyntheticHand_WristLockMode lockMode,
      unsigned char skipAnimation) {
    isdk_SyntheticHand_lockWristPose(
        getSyntheticHandHandle(), wristPose, overrideFactor, lockMode, skipAnimation);
  }

  void lockWristPosition(
      const ovrpVector3f* position,
      const float overrideFactor,
      unsigned char skipAnimation) {
    isdk_SyntheticHand_lockWristPosition(
        getSyntheticHandHandle(), position, overrideFactor, skipAnimation);
  }

  void lockWristRotation(
      const ovrpQuatf* rotation,
      const float overrideFactor,
      unsigned char skipAnimation) {
    isdk_SyntheticHand_lockWristRotation(
        getSyntheticHandHandle(), rotation, overrideFactor, skipAnimation);
  }

  void freeWrist(isdk_SyntheticHand_WristLockMode lockMode) {
    isdk_SyntheticHand_freeWrist(getSyntheticHandHandle(), lockMode);
  }
  [[nodiscard]] isdk_SyntheticHand* getSyntheticHandHandle() const {
    return asSyntheticHand_;
  }
  operator isdk_SyntheticHand*() const {
    return asSyntheticHand_;
  }
  [[nodiscard]] isdk_IHandDataSource* getIHandDataSourceHandle() const override {
    return asIHandDataSource_;
  }
  operator isdk_IHandDataSource*() const {
    return asIHandDataSource_;
  }
  [[nodiscard]] isdk_IHandDataModifier* getIHandDataModifierHandle() const override {
    return asIHandDataModifier_;
  }
  operator isdk_IHandDataModifier*() const {
    return asIHandDataModifier_;
  }

 protected:
  isdk_SyntheticHand* asSyntheticHand_;
  isdk_IHandDataSource* asIHandDataSource_;
  isdk_IHandDataModifier* asIHandDataModifier_;
};
using SyntheticHandPtr = ObjectPtr<SyntheticHand, isdk_SyntheticHand>;

class ThumbRecognizer final : public DigitRecognizer {
 public:
  explicit ThumbRecognizer(isdk_ThumbRecognizer* handle = nullptr)
      : asThumbRecognizer_(handle),
        asIActiveState_(tryInitWithCast<isdk_ThumbRecognizer, isdk_IActiveState>(
            handle,
            &isdk_ThumbRecognizer_castToIActiveState)),
        asIDeltaTimeUpdate_(tryInitWithCast<isdk_ThumbRecognizer, isdk_IDeltaTimeUpdate>(
            handle,
            &isdk_ThumbRecognizer_castToIDeltaTimeUpdate)),
        asDigitRecognizer_(tryInitWithCast<isdk_ThumbRecognizer, isdk_DigitRecognizer>(
            handle,
            &isdk_ThumbRecognizer_castToDigitRecognizer)) {}
  static ObjectPtr<ThumbRecognizer, isdk_ThumbRecognizer> create(
      isdk_IHandPositionFrame* handPositionFrame,
      const isdk_DigitRangeParams* parameters,
      isdk_Detection_ThumbCalcType calcType) {
    isdk_ThumbRecognizer* retVal{};
    isdk_ThumbRecognizer_create(handPositionFrame, parameters, calcType, &retVal);
    return ObjectPtr<ThumbRecognizer, isdk_ThumbRecognizer>(retVal, &isdk_ThumbRecognizer_destroy);
  }
  static void destroy(isdk_ThumbRecognizer* ptr) {
    isdk_ThumbRecognizer_destroy(ptr);
  }
  static isdk_ThumbRecognizer_ExpectedThumbValueRanges getExpectedThumbRawValueRanges() {
    isdk_ThumbRecognizer_ExpectedThumbValueRanges retVal{};
    isdk_ThumbRecognizer_getExpectedThumbRawValueRanges(&retVal);
    return retVal;
  }

  static isdk_DigitRangeParams getThumbRangeParamsFromNormalized(
      const isdk_DigitRangeParams* normalizedParams,
      isdk_Detection_ThumbCalcType calcType) {
    isdk_DigitRangeParams retVal{};
    isdk_ThumbRecognizer_getThumbRangeParamsFromNormalized(normalizedParams, calcType, &retVal);
    return retVal;
  }
  [[nodiscard]] isdk_ThumbRecognizer* getThumbRecognizerHandle() const {
    return asThumbRecognizer_;
  }
  operator isdk_ThumbRecognizer*() const {
    return asThumbRecognizer_;
  }
  [[nodiscard]] isdk_IActiveState* getIActiveStateHandle() const override {
    return asIActiveState_;
  }
  operator isdk_IActiveState*() const {
    return asIActiveState_;
  }
  [[nodiscard]] isdk_IDeltaTimeUpdate* getIDeltaTimeUpdateHandle() const override {
    return asIDeltaTimeUpdate_;
  }
  operator isdk_IDeltaTimeUpdate*() const {
    return asIDeltaTimeUpdate_;
  }
  [[nodiscard]] isdk_DigitRecognizer* getDigitRecognizerHandle() const override {
    return asDigitRecognizer_;
  }
  operator isdk_DigitRecognizer*() const {
    return asDigitRecognizer_;
  }

 protected:
  isdk_ThumbRecognizer* asThumbRecognizer_;
  isdk_IActiveState* asIActiveState_;
  isdk_IDeltaTimeUpdate* asIDeltaTimeUpdate_;
  isdk_DigitRecognizer* asDigitRecognizer_;
};
using ThumbRecognizerPtr = ObjectPtr<ThumbRecognizer, isdk_ThumbRecognizer>;

} 
}// namespace isdk::api
#endif // ISDK_API_HPP
