#pragma once

#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "PlotJuggler/transform_function.h"
#include "ui_moving_average_filter.h"
#include "PlotJuggler/ring_span.hpp"

using namespace PJ;


namespace Ui {
class MovingAverageFilter;
}

class MovingAverageFilter: public TimeSeriesTransform
{
public:
  explicit MovingAverageFilter();

  ~MovingAverageFilter() override;

  const char* name() const override { return "Moving Average"; }

  void calculate(PlotData* dst_data) override;

  QWidget* optionsWidget() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;


private:
  Ui::MovingAverageFilter *ui;
  QWidget *_widget;
  std::vector<double> _buffer;
  nonstd::ring_span_lite::ring_span<double> _ring_view;
};

