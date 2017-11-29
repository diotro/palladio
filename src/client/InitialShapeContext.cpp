#include "InitialShapeContext.h"

#include "GU/GU_Detail.h"

#include "boost/variant.hpp"
#include "boost/algorithm/string.hpp"


namespace {
	const UT_String CE_SHAPE_CLS_NAME   = "ceShapeClsName";
	const UT_String CE_SHAPE_CLS_TYPE   = "ceShapeClsType"; // ordinal of GA_StorageClass
	const UT_String CE_SHAPE_RPK        = "ceShapeRPK";
	const UT_String CE_SHAPE_RULE_FILE  = "ceShapeRuleFile";
	const UT_String CE_SHAPE_START_RULE = "ceShapeStartRule";
	const UT_String CE_SHAPE_STYLE      = "ceShapeStyle";
	const UT_String CE_SHAPE_SEED       = "ceShapeSeed";

	const UT_String DEFAULT_CLS_NAME    = "shapeCls";
} // namespace


namespace p4h {

InitialShapeContext::InitialShapeContext()
	: mShapeClsAttrName(DEFAULT_CLS_NAME), mSeed{0} { }

void InitialShapeContext::put(GU_Detail* detail) {
	std::set<std::string> existingPrimitiveAttributes;
	{
		GA_Attribute* a;
		GA_FOR_ALL_PRIMITIVE_ATTRIBUTES(detail, a) {
			existingPrimitiveAttributes.emplace(a->getName().toStdString());
		}
	}

	// set main generation attributes (potentially overwrite existing attributes)
    GA_RWAttributeRef clsAttrNameRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_CLS_NAME, 1));
	GA_RWHandleS clsAttrNameH(clsAttrNameRef);

	GA_RWAttributeRef clsTypeRef(detail->addIntTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_CLS_TYPE, 1));
	GA_RWHandleI clsTypeH(clsTypeRef);

	GA_RWAttributeRef clsNameRef;
	using HandleType = boost::variant<GA_RWHandleF, GA_RWHandleI, GA_RWHandleS>;
	HandleType clsNameH;
	switch (mShapeClsType) {
		case GA_STORECLASS_FLOAT:
			clsNameRef = GA_RWAttributeRef(detail->addFloatTuple(GA_ATTRIB_PRIMITIVE, mShapeClsAttrName.c_str(), 1));
			clsNameH = GA_RWHandleF(clsNameRef);
			break;
		case GA_STORECLASS_INT:
			clsNameRef = GA_RWAttributeRef(detail->addIntTuple(GA_ATTRIB_PRIMITIVE, mShapeClsAttrName.c_str(), 1));
			clsNameH = GA_RWHandleI(clsNameRef);
			break;
		case GA_STORECLASS_STRING:
			clsNameRef = GA_RWAttributeRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, mShapeClsAttrName.c_str(), 1));
			clsNameH = GA_RWHandleS(clsNameRef);
			break;
		default:
			break;
	}

	GA_RWAttributeRef rpkRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_RPK, 1));
	GA_RWHandleS rpkH(rpkRef);

	GA_RWAttributeRef ruleFileRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_RULE_FILE, 1));
	GA_RWHandleS ruleFileH(ruleFileRef);

	GA_RWAttributeRef startRuleRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_START_RULE, 1));
	GA_RWHandleS startRuleH(startRuleRef);

	GA_RWAttributeRef styleRef(detail->addStringTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_STYLE, 1));
	GA_RWHandleS styleH(styleRef);

	GA_RWAttributeRef seedRef(detail->addIntTuple(GA_ATTRIB_PRIMITIVE, CE_SHAPE_SEED, 1));
	GA_RWHandleI seedH(seedRef);

	// create refs for attributes
	std::map<std::wstring, GA_RWAttributeRef> mAttrRefs;
	size_t keyCount = 0;
	const wchar_t* const* cKeys = mRuleAttributeValues->getKeys(&keyCount);
	for (size_t k = 0; k < keyCount; k++) {
		const wchar_t* key = cKeys[k];
		std::string nKey = utils::toOSNarrowFromUTF16(key);

		// strip away style prefix
        auto styleDelimPos = nKey.find('$');
        if (styleDelimPos != std::string::npos)
            nKey.erase(0, styleDelimPos+1);

		// treat incoming attributes which exist as rule attributes as "user overrides"
		if (existingPrimitiveAttributes.count(nKey) > 0)
			continue;

		boost::replace_all(nKey, ".", "_dot_"); // TODO: make this robust

		switch (mRuleAttributeValues->getType(key)) {
			case prt::AttributeMap::PT_FLOAT: {
				GA_RWAttributeRef ar(detail->addFloatTuple(GA_ATTRIB_PRIMITIVE, nKey.c_str(), 1));
				if (ar.isValid())
					mAttrRefs.emplace(key, ar);
				break;
			}
			case prt::AttributeMap::PT_BOOL: {
				mAttrRefs.emplace(key, detail->addIntTuple(GA_ATTRIB_PRIMITIVE, nKey.c_str(), 1));
				break;
			}
			case prt::AttributeMap::PT_STRING: {
				mAttrRefs.emplace(key, detail->addStringTuple(GA_ATTRIB_PRIMITIVE, nKey.c_str(), 1));
				break;
			}
			default:
				break;
		} // switch type
	} // for user keys

	GA_Primitive* prim = nullptr;
	GA_FOR_ALL_PRIMITIVES(detail, prim) {
		const GA_Offset& off = prim->getMapOffset();
		clsAttrNameH.set(off, mShapeClsAttrName.c_str());
		clsTypeH.set(off, mShapeClsType);

		rpkH.set(off, mRPK.string().c_str());
		ruleFileH.set(off, utils::toOSNarrowFromUTF16(mRuleFile).c_str());
		startRuleH.set(off, utils::toOSNarrowFromUTF16(mStartRule).c_str());
		styleH.set(off, utils::toOSNarrowFromUTF16(mStyle).c_str());
		seedH.set(off, mSeed); // TODO: setting all initial shape primitives to same seed is not what we want (how does CE do it?)

		size_t keyCount = 0;
		const wchar_t* const* cKeys = mRuleAttributeValues->getKeys(&keyCount);
		for (size_t k = 0; k < keyCount; k++) {
			const wchar_t* const key = cKeys[k];
			std::string nKey = utils::toOSNarrowFromUTF16(key);

			// strip away style prefix
			const auto styleDelimPos = nKey.find('$');
			if (styleDelimPos != std::string::npos)
				nKey.erase(0, styleDelimPos+1);

			// treat incoming attributes which exist as rule attributes as "user overrides"
			if (existingPrimitiveAttributes.count(nKey) > 0)
				continue;

			switch (mRuleAttributeValues->getType(key)) {
				case prt::AttributeMap::PT_FLOAT: {
					GA_RWHandleF av(mAttrRefs.at(key)); // TODO: we should stay in double precision here
					if (av.isValid()) {
						const double defVal = mRuleAttributeValues->getFloat(key);
						av.set(off, (fpreal32) defVal); // TODO: again, stay in double precision
					}
					break;
				}
				case prt::AttributeMap::PT_BOOL: {
					GA_RWHandleI av(mAttrRefs.at(key));
					const bool defVal = mRuleAttributeValues->getBool(key);
					av.set(off, defVal ? 1 : 0);
					break;
				}
				case prt::AttributeMap::PT_STRING: {
					GA_RWHandleS av(mAttrRefs.at(key));
					const wchar_t* const defVal = mRuleAttributeValues->getString(key);
					const std::string nDefVal = utils::toOSNarrowFromUTF16(defVal); // !!!
					av.set(off, nDefVal.c_str());
					break;
				}
				default: {
					LOG_ERR << "Array attribute support not implemented yet";
				}
			} // switch type
		} // for user keys
	} // for all prims
}

GA_ROAttributeRef InitialShapeContext::getClsName(const GU_Detail* detail) {
	return detail->findPrimitiveAttribute(CE_SHAPE_CLS_NAME);
}

GA_ROAttributeRef InitialShapeContext::getClsType(const GU_Detail* detail) {
	return detail->findPrimitiveAttribute(CE_SHAPE_CLS_TYPE);
}

} // namespace p4h

