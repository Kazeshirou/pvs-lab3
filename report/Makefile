#
# Простейший Makefile для создания отчёта
#

PDF=report.pdf

# Используемые программы
PDFLATEX = pdflatex -interaction=nonstopmode

LAB=../net
ECHO_CLIENT=$(LAB)/c1/root/echocl.c
ECHO_SERVER=$(LAB)/c4/root/echosrv.c
ECHO_CLIENT_TEX=$(INCLUDES)/echo_client.tex
ECHO_SERVER_TEX=$(INCLUDES)/echo_server.tex

TEX=tex
INCLUDES = $(TEX)/includes
UTILS= utils

SRC2TEX = $(UTILS)/src2tex

.PHONY: clean
all : $(PDF)
report : $(PDF)

$(ECHO_SERVER_TEX): $(ECHO_SERVER)
	$(SRC2TEX) $< 1 | grep -v descrip > $@

$(ECHO_CLIENT_TEX): $(ECHO_CLIENT)
	$(SRC2TEX) $< 1 | grep -v descrip > $@

$(INCLUDES)/%.pdf : $(INCLUDES)/%.tex
	$(PDFLATEX) -output-directory=$(INCLUDES) $<

# Основная цель сборки --- отчет
$(PDF) : $(TEX)/report.tex $(TEX)/header.tex $(ECHO_SERVER_TEX) $(ECHO_CLIENT_TEX)
	cd $(TEX) && $(PDFLATEX) report.tex && $(PDFLATEX) report.tex && cp $(PDF) ..

# Удаление ВСЕХ временных файлов
clean:
	find $(TEX)/ -regextype posix-egrep -type f ! -regex ".*\.(sty|tex|clo|cls|bib|bst|gitignore)" -exec $(RM) {} \; 
	rm -rf $(INCLUDES)/*;

