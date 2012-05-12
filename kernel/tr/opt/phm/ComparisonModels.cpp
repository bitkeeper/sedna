#include "ComparisonModels.h"
#include "tr/opt/cost/Statistics.h"
#include "tr/opt/phm/PhysicalModel.h"

#define CDGQNAME(N) xsd::QName::getConstantQName(NULL_XMLNS, N)

SequenceInfo* GeneralComparisonPrototype::getSequenceCost(CostModel* model, TupleRef in)
{
    if (in->statistics == NULL) {
        U_ASSERT(false);
        return NULL;
    };

    model->getValueCost(in->statistics->pathInfo, in->statistics);
    in->statistics = new TupleStatistics(in->statistics);

    return model->getValueSequenceCost(in);
}

ComparisonInfo* GeneralComparisonPrototype::getComparisonCost(CostModel* model, TupleRef left, TupleRef right)
{
    return model->getCmpInfo(left->statistics, right->statistics, cmp);
}

IElementProducer* GeneralComparisonPrototype::__toXML(IElementProducer* element) const
{
    IElementProducer* el = element->addElement(CDGQNAME("path"));
    el->addText(text_source_cstr(cmp.toLRString().c_str()));
    el->close();

    return element;
}

ICollationTupleSerializer* GeneralComparisonPrototype::createTupleSerializer(unsigned idx)
{
    return new GeneralCollationSerializer(idx);
}

TupleCellComparison GeneralComparisonPrototype::getTupleCellComparison()
{
    switch (cmp.op) {
      case Comparison::g_eq :
        return TupleCellComparison(op_lt, op_eq, true);
      default :
        U_ASSERT(false);
        return TupleCellComparison(NULL, NULL, false);
    };
}

ValueFunction GeneralComparisonPrototype::getValueFunction(unsigned idxL, unsigned idxR)
{
    switch (cmp.op) {
      case Comparison::g_eq :
        return ValueFunction(op_eq, idxL, idxR, true);
      default :
        U_ASSERT(false);
        return ValueFunction(NULL, 0, false);
    };
}



ComparisonInfo* PathComparisonPrototype::getComparisonCost(CostModel* model, TupleRef left, TupleRef right)
{
    return model->getDocOrderInfo(left->statistics->pathInfo, right->statistics->pathInfo, path);
}

SequenceInfo* PathComparisonPrototype::getSequenceCost(CostModel* model, TupleRef in)
{
    if (in->statistics == NULL) {
        U_ASSERT(false);
        return NULL;
    };

    in->statistics = new TupleStatistics(in->statistics);

    return model->getDocOrderSequenceCost(in);
}

IElementProducer* PathComparisonPrototype::__toXML(IElementProducer* element) const
{
    IElementProducer* el = element->addElement(CDGQNAME("cmp"));
    el->addText(text_source_cstr(path.toXPathString().c_str()));
    el->close();

    return element;
}

ValueFunction PathComparisonPrototype::getValueFunction(unsigned int idxL, unsigned int idxR)
{
    pe::Step step = path.getBody()->at(0);

    // TODO : implement all possible axes right

    switch (step.getAxis()) {
      case pe::axis_preceding :
        return ValueFunction(op_doc_order_lt, idxL, idxR, false);
      case pe::axis_following :
        return ValueFunction(op_doc_order_gt, idxL, idxR, false);
      case pe::axis_child :
      case pe::axis_descendant :
      case pe::axis_attribute :
        return ValueFunction(op_doc_order_descendant, idxL, idxR, false);
      case pe::axis_ancestor :
      case pe::axis_parent :
        return ValueFunction(op_doc_order_ancestor, idxL, idxR, false);
      default:
        U_ASSERT(false);
        return ValueFunction(op_doc_order_lt, idxL, idxR, false);
    }
}

ICollationTupleSerializer* PathComparisonPrototype::createTupleSerializer(unsigned idx)
{
    return new DocOrderSerializer(idx);
}


TupleCellComparison PathComparisonPrototype::getTupleCellComparison()
{
    pe::Step step = path.getBody()->at(0);

    // TODO : implement all possible axes right

    switch (step.getAxis()) {
      case pe::axis_preceding :
        return TupleCellComparison(op_doc_order_lt, op_doc_order_lt, false);
      case pe::axis_following :
        return TupleCellComparison(op_doc_order_lt, op_doc_order_gt, false);
      case pe::axis_child :
      case pe::axis_descendant :
      case pe::axis_attribute :
        return TupleCellComparison(op_doc_order_lt, op_doc_order_descendant, false);
      case pe::axis_ancestor :
      case pe::axis_parent :
        return TupleCellComparison(op_doc_order_lt, op_doc_order_ancestor, false);
      default:
        U_ASSERT(false);
        return TupleCellComparison(op_doc_order_lt, op_doc_order_lt, false);
    }
}

