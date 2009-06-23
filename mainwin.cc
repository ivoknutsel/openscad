/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"

#include <QMenu>
#include <QMenuBar>
#include <QSplitter>

MainWindow::MainWindow(const char *filename)
{
        root_ctx.functions_p = &builtin_functions;
        root_ctx.modules_p = &builtin_modules;

	root_module = NULL;
	root_node = NULL;
	root_N = NULL;

	if (filename) {
		this->filename = QString(filename);
		setWindowTitle(this->filename);
	} else {
		setWindowTitle("New Document");
	}

	{
		QMenu *menu = menuBar()->addMenu("&File");
		menu->addAction("&New", this, SLOT(actionNew()));
		menu->addAction("&Open...", this, SLOT(actionOpen()));
		menu->addAction("&Save", this, SLOT(actionSave()));
		menu->addAction("Save &As...", this, SLOT(actionSaveAs()));
		menu->addAction("&Quit", this, SLOT(close()));
	}

	{
		QMenu *menu = menuBar()->addMenu("&Design");
		menu->addAction("&Compile", this, SLOT(actionCompile()));
#ifdef ENABLE_CGAL
		menu->addAction("Compile and &Render (CGAL)", this, SLOT(actionRenderCGAL()));
#endif
		menu->addAction("Display &AST...", this, SLOT(actionDisplayAST()));
		menu->addAction("Display &CSG...", this, SLOT(actionDisplayCSG()));
		menu->addAction("Export as &STL...", this, SLOT(actionExportSTL()));
		menu->addAction("Export as &OFF...", this, SLOT(actionExportOFF()));
	}

	{
		QMenu *menu = menuBar()->addMenu("&View");
		menu->addAction("OpenCSG");
		menu->addAction("CGAL Surfaces");
		menu->addAction("CGAL Grid Only");
		menu->addSeparator();
		menu->addAction("Top");
		menu->addAction("Bottom");
		menu->addAction("Left");
		menu->addAction("Right");
		menu->addAction("Front");
		menu->addAction("Back");
		menu->addAction("Diagonal");
		menu->addSeparator();
		menu->addAction("Perspective");
		menu->addAction("Orthogonal");
	}

	s1 = new QSplitter(Qt::Horizontal, this);
	editor = new QTextEdit(s1);
	s2 = new QSplitter(Qt::Vertical, s1);
	screen = new GLView(s2);
	console = new QTextEdit(s2);

	console->setReadOnly(true);
	console->append("OpenSCAD (www.openscad.at)");
	console->append("Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>");
	console->append("");
	console->append("This program is free software; you can redistribute it and/or modify");
	console->append("it under the terms of the GNU General Public License as published by");
	console->append("the Free Software Foundation; either version 2 of the License, or");
	console->append("(at your option) any later version.");
	console->append("");

	editor->setTabStopWidth(30);

	if (filename) {
		QString text;
		FILE *fp = fopen(filename, "rt");
		if (!fp) {
			console->append(QString("Failed to open text file: %1 (%2)").arg(QString(filename), QString(strerror(errno))));
		} else {
			char buffer[513];
			int rc;
			while ((rc = fread(buffer, 1, 512, fp)) > 0) {
				buffer[rc] = 0;
				text += buffer;
			}
			fclose(fp);
		}
		editor->setPlainText(text);
	}

	screen->polygons.clear();
	screen->polygons.append(GLView::Polygon() << GLView::Point(0,0,0) << GLView::Point(1,0,0) << GLView::Point(0,1,0));
	screen->polygons.append(GLView::Polygon() << GLView::Point(0,0,0) << GLView::Point(1,0,0) << GLView::Point(0,0,1));
	screen->polygons.append(GLView::Polygon() << GLView::Point(1,0,0) << GLView::Point(0,1,0) << GLView::Point(0,0,1));
	screen->polygons.append(GLView::Polygon() << GLView::Point(0,1,0) << GLView::Point(0,0,0) << GLView::Point(0,0,1));
	screen->updateGL();

	setCentralWidget(s1);
}

MainWindow::~MainWindow()
{
	if (root_module)
		delete root_module;
	if (root_node)
		delete root_node;
	if (root_N)
		delete root_N;
}

void MainWindow::actionNew()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionOpen()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionSave()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionSaveAs()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionCompile()
{
	if (root_module) {
		delete root_module;
		root_module = NULL;
	}

	console->append("Parsing design (AST generation)...");
	root_module = parse(editor->toPlainText().toAscii().data(), false);

	if (!root_module) {
		console->append("Compilation failed!");
		return;
	}

	if (root_node) {
		delete root_node;
		root_node = NULL;
	}

	console->append("Compiling design (CSG generation)...");
	root_node = root_module->evaluate(&root_ctx, QVector<QString>(), QVector<Value>(), QVector<AbstractNode*>());

	if (!root_node) {
		console->append("Compilation failed!");
		return;
	}

	console->append("Compilation finished.");
}

#ifdef ENABLE_CGAL

static void report_func(const class AbstractNode*, void *vp, int mark)
{
	MainWindow *m = (MainWindow*)vp;
	QString msg;
	msg.sprintf("CSG rendering progress: %.2f%%", (mark*100.0) / progress_report_count);
        m->console->append(msg);
}

#include <CGAL/Nef_3/OGL_helper.h>

static void renderGLviaCGAL(void *vp)
{
	MainWindow *m = (MainWindow*)vp;

	CGAL::OGL::Polyhedron P;
	CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron>::convert_to_OGLPolyhedron(*m->root_N, &P);
	P.draw();
}

void MainWindow::actionRenderCGAL()
{
	actionCompile();

	if (!root_module || !root_node)
		return;

	if (root_N) {
		delete root_N;
		root_N = NULL;
	}

	progress_report_prep(root_node, report_func, this);
	root_N = new CGAL_Nef_polyhedron(root_node->render_cgal_nef_polyhedron());
	progress_report_fin();

	screen->polygons.clear();
	screen->renderfunc = renderGLviaCGAL;
	screen->renderfunc_vp = this;
	screen->updateGL();
}

#endif /* ENABLE_CGAL */

void MainWindow::actionDisplayAST()
{
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("AST Dump");
	if (root_module) {
		e->setPlainText(root_module->dump("", ""));
	} else {
		e->setPlainText("No AST to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
}

void MainWindow::actionDisplayCSG()
{
	QTextEdit *e = new QTextEdit(NULL);
	e->setTabStopWidth(30);
	e->setWindowTitle("CSG Dump");
	if (root_node) {
		e->setPlainText(root_node->dump(""));
	} else {
		e->setPlainText("No CSG to dump. Please try compiling first...");
	}
	e->show();
	e->resize(600, 400);
}

void MainWindow::actionExportSTL()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}

void MainWindow::actionExportOFF()
{
	console->append(QString("Function %1 is not implemented yet!").arg(QString(__PRETTY_FUNCTION__)));
}


