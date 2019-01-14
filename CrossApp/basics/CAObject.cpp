

#include "CAObject.h"
#include "basics/CAAutoreleasePool.h"
#include "ccMacros.h"
#include "basics/CAScheduler.h"
#include "basics/CANotificationCenter.h"
#include "script_support/CAScriptSupport.h"

NS_CC_BEGIN

std::map<void *, int> CAObject::needCheckObjVecs;
std::map<void *, int> CAObject::canCheckObjVecs;
CAObject::CAObject(void)
: m_uReference(1) // when the object is created, the reference count of it is 1
, m_uAutoReleaseCount(0)
, m_nTag(TagInvalid)
, m_pUserData(nullptr)
, m_pUserObject(nullptr)
{
    static unsigned int uObjectCount = 0;

    m_u__ID = ++uObjectCount;

    char str[8];
    sprintf(str, "%x", m_u__ID);
    m_s__StrID = str;
}

CAObject::~CAObject(void)
{
	if (CANotificationCenter* notificationCenter = CANotificationCenter::getInstance())
	{
		notificationCenter->removeAllObservers(this);
	}
	if (CAScheduler* scheduler = CAScheduler::getScheduler())
	{
		scheduler->unscheduleAllForTarget(this);;
	}
    
    CC_SAFE_RELEASE(m_pUserObject);
    
    if (m_uAutoReleaseCount > 0)
    {
        CAPoolManager::getInstance()->removeObject(this);
    }
    
    CAScriptEngineManager* pEngineManager = CAScriptEngineManager::getScriptEngineManager();
    if (pEngineManager)
    {
        CAScriptEngineProtocol* pEngine = pEngineManager->getScriptEngine();
        if (pEngine != NULL && pEngine->getScriptType() == kScriptTypeJavascript)
        {
            pEngine->removeScriptObjectByObject(this);
        }
    }
}

void CAObject::release(void)
{
    --m_uReference;

    if (m_uReference == 0)
    {
		delete this;
    }
}

CAObject* CAObject::retain(void)
{
    ++m_uReference;
    return this;
}

CAObject* CAObject::autorelease(void)
{
    CAPoolManager::getInstance()->addObject(this);
    return this;
}

bool CAObject::isSingleReference(void) const
{
    return m_uReference == 1;
}

unsigned int CAObject::retainCount(void) const
{
    return m_uReference;
}

bool CAObject::isEqual(const CAObject *pObject)
{
    return this == pObject;
}

void CAObject::performSelector(SEL_CallFunc callFunc, float afterDelay)
{
    CAScheduler::getScheduler()->scheduleOnce([=](float dt)
    {
        (this->*callFunc)();
    }, crossapp_format_string("perform:%x", callFunc), this, afterDelay);
}

void CAObject::performSelector(SEL_CallFuncO callFunc, CAObject* objParam, float afterDelay)
{
    CAScheduler::getScheduler()->scheduleOnce([&](float dt)
    {
        (this->*callFunc)(objParam);
    }, crossapp_format_string("performO:%x", callFunc), this, afterDelay);
}

void CAObject::cancelPreviousPerformRequests(SEL_CallFunc callFunc)
{
    CAScheduler::getScheduler()->unschedule(crossapp_format_string("perform:%x", callFunc), this);
}

void CAObject::cancelPreviousPerformRequests(SEL_CallFuncO callFunc)
{
    CAScheduler::getScheduler()->unschedule(crossapp_format_string("performO:%x", callFunc), this);
}

void CAObject::setCheckExist(bool bCheck)
{
    CCLog("setCheckExist %p", this);
    if (bCheck)
    {
        needCheckObjVecs[this] = 1;
        canCheckObjVecs[this]=1;
    }
    else
    {
        canCheckObjVecs[this]=0;
        removeCheck(this);
    }
}
bool CAObject::canCheckExist(void * pObj)
{
    //CCLog("%p", pObj);
    if (pObj == NULL)
    {
        return false;
    }
    auto ite = canCheckObjVecs.find(pObj);
    if (ite != canCheckObjVecs.end() && ite->second == 1)
    {
        return true;
    }
    return  false;
}
bool CAObject::isExist(void * pObj)
{
    //CCLog("%p", pObj);
    if (pObj == NULL)
    {
        return false;
    }
    auto ite = needCheckObjVecs.find(pObj);
    if (ite != needCheckObjVecs.end() && ite->second == 1)
    {
        return true;
    }
    return  false;
}
bool CAObject::removeCheck(void * pObj)
{
    std::map<void *, int>::iterator ite=needCheckObjVecs.find(pObj);
    if (ite!= needCheckObjVecs.end())
    {
        needCheckObjVecs[pObj] = 0;
        needCheckObjVecs.erase(ite);
    }
    return true;
}
NS_CC_END
