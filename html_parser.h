#include <string_view>
#include <string>
#include <variant>
#include <vector>

enum class parser_error
{
	unexpected_null_character,
	unexpected_question_mark_instead_of_tag_name,
	eof_before_tag_name,
	eof_in_tag,
	invalid_first_character_of_tag_name,
	missing_end_tag_name,
	unexpected_equal_sign_before_attribute_name,
	unexpected_character_in_attribute_name,
	unexpected_character_in_unquoted_attribute_value,
	missing_attribute_value,
	unexpected_solidus_in_tag,
};

struct character_token
{
	std::string value;
};

struct comment
{
	std::string value;
};

struct char_entity
{
	char const * name;
	char const * value;
};

static char_entity const entities[] = {
	{ "&amp", "&" },
};

struct tag
{
	enum
	{
		start,
		end,
	} kind;

	bool self_closing = false;

	std::string name;
	std::vector<std::pair<std::string, std::string>> attrs;
};

using token = std::variant<parser_error, character_token, comment, tag>;

struct tokenizer
{
	void run(std::string_view & input, token *& out_first, token * out_last)
	{
		auto new_attr = [this]() {
			open_tag_.attrs.emplace_back();
			open_attr_ = &open_tag_.attrs.back();
		};

		while (!input.empty() && out_first != out_last)
		{
			char ch = input.front();

			bool reconsume = false;

			switch (state_)
			{
			case state::data:
				switch (ch)
				{
				case '&':
					return_state_ = state::data;
					state_ = state::character_reference;
					break;
				case '<':
					state_ = state::tag_open;
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					// fallthrough
				default:
					*out_first++ = character_token{ std::string(1, ch) };
				}
				break;
			case state::tag_open:
				if (('A' <= ch && ch <= 'Z')
					|| ('a' <= ch && ch <= 'z'))
				{
					open_tag_.name.clear();
					open_tag_.kind = tag::start;
					state_ = state::tag_name;
					reconsume = true;
				}
				else
				{
					switch (ch)
					{
					case '!':
						state_ = state::markup_declaration_open;
						break;
					case '/':
						state_ = state::end_tag_open;
						break;
					case '?':
						err(parser_error::unexpected_question_mark_instead_of_tag_name);
						state_ = state::bogus_comment;
						reconsume = true;
						break;
					default:
						err(parser_error::invalid_first_character_of_tag_name);
						state_ = state::data;
						reconsume = true;
						break;
					}
				}
				break;
			case state::end_tag_open:
				if (('A' <= ch && ch <= 'Z')
					|| ('a' <= ch && ch <= 'z'))
				{
					open_tag_.name.clear();
					open_tag_.kind = tag::end;
					state_ = state::tag_name;
					reconsume = true;
				}
				else
				{
					switch (ch)
					{
					case '>':
						err(parser_error::missing_end_tag_name);
						state_ = state::data;
						break;
					default:
						err(parser_error::invalid_first_character_of_tag_name);
						*out_first++ = comment{ "" };
						state_ = state::bogus_comment;
						reconsume = true;
						break;
					}
				}
				break;
			case state::tag_name:
				if ('A' <= ch && ch <= 'Z')
				{
					open_tag_.name.push_back(ch + ('a' - 'A'));
				}
				else if ('a' <= ch && ch <= 'z')
				{
					open_tag_.name.push_back(ch);
				}
				else
				{
					switch (ch)
					{
					case '\t':
					case '\n':
					case '\f':
					case ' ':
						state_ = state::before_attribute_name;
						break;
					case '/':
						state_ = state::self_closing_start_tag;
						break;
					case '>':
						*out_first++ = open_tag_;
						state_ = state::data;
						break;
					case 0:
						err(parser_error::unexpected_null_character);
						open_tag_.name.append(u8"\ufffd");
						break;
					default:
						open_tag_.name.push_back(ch);
						break;
					}
				}
				break;
			case state::before_attribute_name:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
					break;
				case '/':
				case '>':
					state_ = state::after_attribute_name;
					reconsume = true;
					break;
				case '=':
					err(parser_error::unexpected_equal_sign_before_attribute_name);
					break;
				default:
					open_attr_name_.clear();
					open_attr_val_.clear();
					state_ = state::attribute_name;
					break;
				}
				break;
			case state::attribute_name:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
				case '/':
				case '>':
					reconsume = true;
					state_ = state::after_attribute_name;
					break;
				case '=':
					state_ = state::before_attribute_value;
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					open_attr_name_.append(u8"\ufffd");
					break;
				case '"':
				case '\'':
				case '<':
					err(parser_error::unexpected_character_in_attribute_name);
					// fallthrough
				default:
					if ('A' <= ch && ch <= 'Z')
						open_attr_name_.push_back(ch + ('a' - 'A'));
					else
						open_attr_name_.push_back(ch);
					break;
				}
				break;
			case state::before_attribute_value:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
					break;
				case '"':
					state_ = state::attribute_value_double_quoted;
					break;
				case '\'':
					state_ = state::attribute_value_single_quoted;
					break;
				case '>':
					err(parser_error::missing_attribute_value);
					state_ = state::data;
					*out_first++ = open_tag_;
					break;
				default:
					state_ = state::attribute_value_unquoted;
					reconsume = true;
				}
				break;
			case state::attribute_value_double_quoted:
				switch (ch)
				{
				case '"':
					state_ = state::after_attribute_value_quoted;
					reconsume = true;
					break;
				case '&':
					return_state_ = state::attribute_value_double_quoted;
					state_ = state::character_reference;
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					open_attr_val_.append(u8"\ufffd");
					break;
				default:
					open_attr_val_.push_back(ch);
				}
				break;
			case state::attribute_value_single_quoted:
				switch (ch)
				{
				case '\'':
					state_ = state::after_attribute_value_quoted;
					reconsume = true;
					break;
				case '&':
					return_state_ = state::attribute_value_single_quoted;
					state_ = state::character_reference;
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					open_attr_val_.append(u8"\ufffd");
					break;
				default:
					open_attr_val_.push_back(ch);
				}
				break;
			case state::attribute_value_unquoted:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
					state_ = state::before_attribute_name;
					break;
				case '&':
					return_state_ = state::attribute_value_unquoted;
					state_ = state::character_reference;
					break;
				case '>':
					state_ = state::data;
					*out_first++ = open_tag_;
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					open_attr_->second.append(u8"\ufffd");
					break;
				case '"':
				case '\'':
				case '<':
				case '=':
				case '`':
					err(parser_error::unexpected_character_in_unquoted_attribute_value);
					// fallthrough
				default:
					open_attr_val_.push_back(ch);
				}
				break;
			case state::after_attribute_value_quoted:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
					state_ = state::before_attribute_name;
					break;
				case '/':
					state_ = state::self_closing_start_tag;
					break;
				case '>':
					state_ = state::data;
					*out_first++ = open_tag_;
					break;
				default:
					state_ = state::before_attribute_name;
					reconsume = true;
				}
				break;
			case state::after_attribute_name:
				switch (ch)
				{
				case '\t':
				case '\n':
				case '\f':
				case ' ':
					break;
				case '/':
					state_ = state::self_closing_start_tag;
					break;
				case '=':
					state_ = state::before_attribute_value;
					break;
				case '>':
					state_ = state::data;
					*out_first++ = open_tag_;
					break;
				default:
					new_attr();
					state_ = state::attribute_name;
					reconsume = true;
				}
				break;
			case state::self_closing_start_tag:
				switch (ch)
				{
				case '>':
					open_tag_.self_closing = true;
					state_ = state::data;
					*out_first++ = open_tag_;
					break;
				default:
					err(parser_error::unexpected_solidus_in_tag);
					state_ = state::before_attribute_name;
					reconsume = true;
				}
				break;
			case state::bogus_comment:
				switch (ch)
				{
				case '>':
					state_ = state::data;
					*out_first++ = comment{ open_tag_.name };
					break;
				case 0:
					err(parser_error::unexpected_null_character);
					open_tag_.name.append(u8"\ufffd");
					break;
				default:
					open_tag_.name.push_back(ch);
				}
				break;
			case state::character_reference:
				ref_ = "&";
				if (('0' <= ch && ch <= '9')
					|| ('a' <= ch && ch <= 'z')
					|| ('A' <= ch && ch <= 'Z'))
				{
					state_ = state::named_character_reference;
					reconsume = true;
				}
				else if (ch == '#')
				{
					state_ = state::numeric_character_reference;
					ref_.push_back(ch);
				}
				else
				{
					state_ = return_state_;
				}
				break;
			case state::named_character_reference:
				break;
			}
		}
	}

	bool finalize(token *& out_first, token * out_last)
	{
		switch (state_)
		{
		case state::data:
			return true;
		case state::tag_open:
			err(parser_error::eof_before_tag_name);
			*out_first++ = character_token{ "<" };
			return true;
		case state::end_tag_open:
			err(parser_error::eof_before_tag_name);
			*out_first++ = character_token{ "</" };
			return true;
		case state::tag_name:
			err(parser_error::eof_in_tag);
			return true;
		case state::before_attribute_name:
		case state::attribute_name:
			state_ = state::after_attribute_name;
			break;
		case state::before_attribute_value:
			state_ = state::attribute_value_unquoted;
			break;
		case state::attribute_value_double_quoted:
		case state::attribute_value_single_quoted:
		case state::attribute_value_unquoted:
		case state::after_attribute_value_quoted:
		case state::after_attribute_name:
		case state::self_closing_start_tag:
			err(parser_error::eof_in_tag);
			return true;
		case state::bogus_comment:
			*out_first++ = comment{ open_tag_.name };
			return true;
		case state::character_reference:
			state_ = return_state_;
			break;
		}
	}

private:
	void err(parser_error e)
	{
	}

	enum class state
	{
		data,
		tag_open,
		end_tag_open,
		tag_name,
		before_attribute_name,
		attribute_name,
		before_attribute_value,
		attribute_value_double_quoted,
		attribute_value_single_quoted,
		attribute_value_unquoted,
		after_attribute_value_quoted,
		after_attribute_name,
		self_closing_start_tag,
		bogus_comment,
		character_reference,
		named_character_reference,
		numeric_character_reference,
		markup_declaration_open,
	};

	state state_ = state::data;
	state return_state_;

	std::string ref_;

	tag open_tag_;
	std::pair<std::string, std::string> * open_attr_ = nullptr;
};
