#include "stdafx.h"
#include "GuiComponent.h"
#include "Tesselator.h"

void GuiComponent::hLine(int x0, int x1, int y, int col)
{
    if (x1 < x0)
	{
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    fill(x0, y, x1 + 1, y + 1, col);
}

void GuiComponent::vLine(int x, int y0, int y1, int col)
{
    if (y1 < y0)
	{
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    fill(x, y0 + 1, x + 1, y1, col);
}

void GuiComponent::fill(int x0, int y0, int x1, int y1, int col)
{
    if (x0 < x1)
	{
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (y0 < y1)
	{
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    float a = ((col >> 24) & 0xff) / 255.0f;
    float r = ((col >> 16) & 0xff) / 255.0f;
    float g = ((col >> 8) & 0xff) / 255.0f;
    float b = ((col) & 0xff) / 255.0f;
    Tesselator *t = Tesselator::getInstance();
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    t->begin();
    t->vertex(static_cast<float>(x0), static_cast<float>(y1), static_cast<float>(0));
    t->vertex(static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(0));
    t->vertex(static_cast<float>(x1), static_cast<float>(y0), static_cast<float>(0));
    t->vertex(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(0));
    t->end();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void GuiComponent::fillGradient(int x0, int y0, int x1, int y1, int col1, int col2)
{
    float a1 = ((col1 >> 24) & 0xff) / 255.0f;
    float r1 = ((col1 >> 16) & 0xff) / 255.0f;
    float g1 = ((col1 >> 8) & 0xff) / 255.0f;
    float b1 = ((col1) & 0xff) / 255.0f;

    float a2 = ((col2 >> 24) & 0xff) / 255.0f;
    float r2 = ((col2 >> 16) & 0xff) / 255.0f;
    float g2 = ((col2 >> 8) & 0xff) / 255.0f;
    float b2 = ((col2) & 0xff) / 255.0f;
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);

    Tesselator *t = Tesselator::getInstance();
    t->begin();
    t->color(r1, g1, b1, a1);
    t->vertex(static_cast<float>(x1), static_cast<float>(y0), blitOffset);
    t->vertex(static_cast<float>(x0), static_cast<float>(y0), blitOffset);
    t->color(r2, g2, b2, a2);
    t->vertex(static_cast<float>(x0), static_cast<float>(y1), blitOffset);
    t->vertex(static_cast<float>(x1), static_cast<float>(y1), blitOffset);
    t->end();

    glShadeModel(GL_FLAT);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
}

GuiComponent::GuiComponent()
{
	blitOffset = 0;
}

void GuiComponent::drawCenteredString(Font *font, const wstring& str, int x, int y, int color)
{
	font->drawShadow(str, x - (font->width(str)) / 2, y, color);
}

void GuiComponent::drawString(Font *font, const wstring& str, int x, int y, int color)
{
	font->drawShadow(str, x, y, color);
}

void GuiComponent::drawStringLiteral(Font *font, const wstring& str, int x, int y, int color)
{
	font->drawShadowLiteral(str, x, y, color);
}

void GuiComponent::blit(int x, int y, int sx, int sy, int w, int h)
{
    
}