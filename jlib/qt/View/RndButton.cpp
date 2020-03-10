#include "RndButton.h"
#include "../QtUtils.h"
#include "../QtStylesheet.h"

using namespace jlib::qt;

RndButton::RndButton(QWidget* parent)
	: QWidget(parent)
{
	txt_ = new QLabel(this);
	txt_->setAlignment(Qt::AlignCenter);
	txt_->hide();
}

RndButton::~RndButton()
{
}

void RndButton::set_attr(QString txt, QSize sz, int font_size)
{
	font_sz_ = font_size;
	txt_->setStyleSheet(build_style(Qt::white, font_size));
	txt_->setText(txt);
	setFixedSize(sz);

	txt_->resize(size());
	txt_->move(0, 0);
	txt_->show();

	bk_color_ = def_colors::control_bk;

	update();
}

void RndButton::set_highlight(bool on)
{
	is_highlighted_ = on;
	bk_color_ = is_highlighted_ ? Qt::lightGray : def_colors::control_bk;
	update();
}

void RndButton::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	QPainterPath path;
	path.addRoundedRect(QRectF(0, 0, width(), height()), 10, 10);
	QPen pen(Qt::black, 1);
	painter.setPen(pen);
	painter.fillPath(path, bk_color_);
	painter.drawPath(path);
}

void RndButton::enterEvent(QEvent* e)
{
	if (!isEnabled()) { return; }
	setCursor(QCursor(Qt::PointingHandCursor));
	bk_color_ = Qt::darkGray;
	update();

	emit sig_focus_on();
}

void RndButton::leaveEvent(QEvent* e)
{
	if (!isEnabled()) { return; }
	setCursor(QCursor(Qt::ArrowCursor));

	bk_color_ = is_highlighted_ ? Qt::lightGray : def_colors::control_bk;
	update();

	is_pressed_ = false;
}

void RndButton::mousePressEvent(QMouseEvent* e)
{
	if (e->button() != Qt::LeftButton)return;
	bk_color_ = def_colors::control_bk;
	update();

	is_pressed_ = true;
}

void RndButton::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() != Qt::LeftButton)return;
	bk_color_ = Qt::darkGray;
	update();

	if (is_pressed_) {
		emit clicked();
		is_pressed_ = false;
	}
}

