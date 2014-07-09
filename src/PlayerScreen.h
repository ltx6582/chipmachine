
#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

class TextScreen {
public:

	struct TextField {
		TextField(const std::string &text, float x, float y, float sc, uint32_t col) : text(text), pos(x, y), scale(sc), color(col), add(0), f {&pos.x, &pos.y, &scale, &color.r, &color.g, &color.b, &color.a, &add} {
		}

		std::string text;
		utils::vec2f pos;

		float scale;
		grappix::Color color;
		float add;

		float& operator[](int i) { return *f[i]; }

		int size() { return 8; }
	private:
		float* f[8];
	};

	void render(uint32_t d) {
		for(auto &f : fields) {
			grappix::screen.text(font, f->text, f->pos.x, f->pos.y, f->color + f->add, f->scale);
		}
	}

	void setFont(const grappix::Font &font) {
		this->font = font;
	}

	grappix::Font& getFont() {
		return font;
	}

	std::shared_ptr<TextField> addField(const std::string &text, float x = 0, float y = 0, float scale = 1.0, uint32_t color = 0xffffffff) {
		fields.push_back(std::make_shared<TextField>(text, x, y, scale, color));
		return fields.back();
	}

	void removeField(const std::shared_ptr<TextField> &field) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(field.get() == it->get())
				it = fields.erase(it);
			else
				it++;
		}
	}

private:

	grappix::Font font;
	std::vector<std::shared_ptr<TextField>> fields;
};