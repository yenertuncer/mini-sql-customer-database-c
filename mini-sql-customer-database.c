#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LEN 100
#define MAX_EMAIL_LEN 100
#define MAX_DATE_LEN 11
#define MAX_LINE_LEN 512
#define MAX_FIELD_LEN 150
#define INITIAL_CAPACITY 10 /* customer table's capacity */

/* JobType Enum */
typedef enum {
    BACKEND_DEVELOPER, FRONTEND_DEVELOPER, FULLSTACK_DEVELOPER, MOBILE_DEVELOPER, EMBEDDED_SOFTWARE_ENGINEER, GAME_DEVELOPER, DEVOPS_ENGINEER, TEST_ENGINEER, JOB_TYPE_COUNT /* Used for enum count and loop bounds */
} JobType;

/* string representations for JobType enum values for printing */
const char *job_type_strings[] = {
    "BACKEND_DEVELOPER", "FRONTEND_DEVELOPER", "FULLSTACK_DEVELOPER", "MOBILE_DEVELOPER", "EMBEDDED_SOFTWARE_ENGINEER", "GAME_DEVELOPER", "DEVOPS_ENGINEER", "TEST_ENGINEER"
};

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct {
    int id;
    char *name;
    char *mail;
    JobType job_type;
    int email_verified;
    Date date_of_birth;
} Customer;

Customer *customer_table = NULL;    /* array storing customer records */
int customer_count = 0;             /* number of customers in the table */
int table_capacity = 0;             /* capacity of the  array */
int next_customer_id = 1;

/* Memory and String */
char *strdup_custom(const char *s);
void trim_whitespace(char *str);
void trim_quotes_if_present(char *str);

/* Data Parsing and Conversion */
void parse_date(const char *date_str, Date *date_obj);
JobType int_to_job_type(int val);
const char *job_type_to_string(JobType jt);

/* Customer Table Management */
void free_customer_fields(Customer *customer);
void ensure_table_capacity(void);
void add_customer_record(Customer customer);

void load_initial_data(const char *filename);
void print_table_to_file(FILE *outfile);
void print_separator_and_table_to_file(const char *output_filename, int is_error_condition, const char* error_message);

void robust_parse_insert_values(const char *values_str_const, char *name_out, char *email_out, int *job_type_out, int *email_verified_out, char *date_str_out);
void handle_insert_command(const char *params);
void handle_delete_command(const char *params);
int handle_update_command(const char *params);
void handle_truncate_command(void);
void process_commands(const char *commands_filename, const char *output_filename);

void cleanup_database(void);

int main() {
    FILE *outfile_init;

    outfile_init = fopen("output.txt", "w");
    if (outfile_init == NULL) {
        perror("Error opening output.txt for initial write");
        return 1;
    }

    load_initial_data("input.txt");
    print_table_to_file(outfile_init);
    fclose(outfile_init);

    process_commands("commands.txt", "output.txt");

    cleanup_database();
    return 0;
}

char *strdup_custom(const char *s) {
    char *d;
    if (!s) return NULL;
    d = (char*)malloc(strlen(s) + 1);
    if (d == NULL) {
        perror("strdup_custom malloc failed");
        exit(EXIT_FAILURE);
    }
    strcpy(d, s);
    return d;
}

void trim_whitespace(char *str) {
    char *start, *end;
    if (str == NULL || *str == '\0') return;

    for (start = str; *start && isspace((unsigned char)*start); ++start);

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    *end = '\0';

    if (start > str) {
        memmove(str, start, (end - start) + 1);
    }
}

void trim_quotes_if_present(char *str) {
    size_t len;
    if (str == NULL) return;
    len = strlen(str);

    if (len >= 2) {
        if (str[0] == '"' && str[len - 1] == '"') {
            str[len - 1] = '\0';
            memmove(str, str + 1, len - 1);
            return;
        }
        if (len >= 6 && (unsigned char)str[0] == 0xE2 && (unsigned char)str[1] == 0x80 && (unsigned char)str[2] == 0x9C && (unsigned char)str[len-3] == 0xE2 && (unsigned char)str[len-2] == 0x80 && (unsigned char)str[len-1] == 0x9D) {
            memmove(str, str + 3, len - 6); 
            str[len - 6] = '\0';            
            return;
        }
    }
}

/* Parses a date string "DD.MM.YYYY" into a Date struct.*/
void parse_date(const char *date_str, Date *date_obj) {
    if (date_str == NULL || strlen(date_str) == 0 || strcmp(date_str, "0.0.0") == 0) {
        date_obj->day = 0; date_obj->month = 0; date_obj->year = 0;
        return;
    }
    if (sscanf(date_str, "%d.%d.%d", &date_obj->day, &date_obj->month, &date_obj->year) != 3) {
        date_obj->day = 0; date_obj->month = 0; date_obj->year = 0;
    }
}

/* Converts an integer to a JobType enum.*/
JobType int_to_job_type(int val) {
    if (val >= BACKEND_DEVELOPER && val < JOB_TYPE_COUNT) return (JobType)val;
    return BACKEND_DEVELOPER;
}

/* Converts a JobType enum to its string representation. */
const char *job_type_to_string(JobType jt) {
    if (jt >= BACKEND_DEVELOPER && jt < JOB_TYPE_COUNT) return job_type_strings[jt];
    return "UNKNOWN_JOB_TYPE";
}

void free_customer_fields(Customer *customer) {
    if (customer->name) { free(customer->name); customer->name = NULL; }
    if (customer->mail) { free(customer->mail); customer->mail = NULL; }
}

/* Ensures customer_table has enough capacity, reallocates if necessary. */
void ensure_table_capacity(void) {
    if (customer_count >= table_capacity) {
        int new_capacity = (table_capacity == 0) ? INITIAL_CAPACITY : table_capacity * 2;
        Customer *new_table = (Customer*)realloc(customer_table, new_capacity * sizeof(Customer));
        if (new_table == NULL) {
            perror("Failed to reallocate customer table");
            cleanup_database();
            exit(EXIT_FAILURE);
        }
        customer_table = new_table;
        table_capacity = new_capacity;
    }
}

/* Adds a customer record to the global customer_table. */
void add_customer_record(Customer customer) {
    ensure_table_capacity();
    customer_table[customer_count++] = customer;
}

/* Loads initial customer data from the file. */
void load_initial_data(const char *filename) {
    FILE *file = fopen(filename, "r");
    char line[MAX_LINE_LEN];
    char *token;
    Customer temp_customer;

    if (!file) { perror("Error opening input file"); return; }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        temp_customer.id = next_customer_id++;

        token = strtok(line, ",");
        temp_customer.name = strdup_custom((token && strlen(token) > 0) ? token : "null");
        token = strtok(NULL, ",");
        temp_customer.mail = strdup_custom((token && strlen(token) > 0) ? token : "null");
        token = strtok(NULL, ",");
        temp_customer.job_type = (token && strlen(token) > 0) ? int_to_job_type(atoi(token)) : BACKEND_DEVELOPER;
        token = strtok(NULL, ",");
        temp_customer.email_verified = (token && strlen(token) > 0) ? atoi(token) : 0;
        token = strtok(NULL, "");
        parse_date((token && strlen(token) > 0) ? token : "0.0.0", &temp_customer.date_of_birth);

        add_customer_record(temp_customer);
    }
    fclose(file);
}

/* print the current customer table. */
void print_table_to_file(FILE *outfile) {
    int i;
    if (!outfile) return;

    for (i = 0; i < customer_count; ++i) {
        Customer c = customer_table[i];
        fprintf(outfile, "%s,%s,%s,%s,%02d.%02d.%04d\n",
                c.name ? c.name : "null",
                c.mail ? c.mail : "null",
                job_type_to_string(c.job_type),
                c.email_verified ? "true" : "false",
                c.date_of_birth.day, c.date_of_birth.month, c.date_of_birth.year);
    }
}

/* Prints the table content to the output file. */
void print_separator_and_table_to_file(const char *output_filename, int is_error_condition, const char* error_message) {
    FILE *outfile = fopen(output_filename, "a");
    if (!outfile) { perror("Error opening output file for append"); return; }

    fprintf(outfile, "----------\n");

    if (is_error_condition) {
        if (error_message) fprintf(outfile, "%s\n", error_message);
    } else {
        print_table_to_file(outfile);
    }
    fclose(outfile);
}

/* Parses INSERT values safely; handles quotes and empty fields, assigns defaults. */
void robust_parse_insert_values(const char *values_str_const, char *name_out, char *email_out,
                                int *job_type_out, int *email_verified_out, char *date_str_out) {
    char values_copy[MAX_LINE_LEN];
    const char *p;
    int field_index = 0;
    char token_buffer[MAX_FIELD_LEN];

    /* Default values for fields */
    strcpy(name_out, "null");
    strcpy(email_out, "null");
    *job_type_out = 0;
    *email_verified_out = 0;
    strcpy(date_str_out, "0.0.0");

    if (values_str_const == NULL || strlen(values_str_const) == 0) return;

    strncpy(values_copy, values_str_const, MAX_LINE_LEN -1);
    values_copy[MAX_LINE_LEN-1] = '\0';

    p = values_copy;
    if (*p == '(') p++;

    /* Parse up to 5 fields or until closing parenthesis or end of string */
    while (*p && *p != ')' && field_index < 5) {
        const char *token_start = p;
        const char *cursor = p; /*temp*/
        int in_quotes = 0;
        size_t token_len;

        /* Find the end of the current token (next comma or closing parenthesis), taking quotes into account. */
        while (*cursor && *cursor != ')') {
            if (*cursor == '"') { /* Standard quote */
                in_quotes = !in_quotes;
                cursor++;
                continue;
            }
            /* Special UTF-8 quotes: “ or ” */
            if ( (unsigned char)cursor[0] == 0xE2 && (unsigned char)cursor[1] == 0x80 &&
                 ((unsigned char)cursor[2] == 0x9C || (unsigned char)cursor[2] == 0x9D) ) {
                in_quotes = !in_quotes;
                cursor += 3;
                continue;
            }
            if (!in_quotes && *cursor == ',') break;
            cursor++;
        }
        token_len = cursor - token_start;

        if (token_len >= MAX_FIELD_LEN) token_len = MAX_FIELD_LEN - 1;
        strncpy(token_buffer, token_start, token_len);
        token_buffer[token_len] = '\0';

        trim_whitespace(token_buffer);
        trim_quotes_if_present(token_buffer);

        if (strlen(token_buffer) > 0) {
            switch(field_index) {
                case 0: strncpy(name_out, token_buffer, MAX_NAME_LEN-1); name_out[MAX_NAME_LEN-1] = '\0'; break;
                case 1: strncpy(email_out, token_buffer, MAX_EMAIL_LEN-1); email_out[MAX_EMAIL_LEN-1] = '\0'; break;
                case 2: *job_type_out = atoi(token_buffer); break;
                case 3: *email_verified_out = atoi(token_buffer); break;
                case 4: strncpy(date_str_out, token_buffer, MAX_DATE_LEN-1); date_str_out[MAX_DATE_LEN-1] = '\0'; break;
            }
        }
        field_index++;
        p = cursor;
        if (*p == ',') p++;
    }
}

/* Handles the INSERT command. */
void handle_insert_command(const char *params) {
    Customer new_customer;
    char name_val[MAX_NAME_LEN], email_val[MAX_EMAIL_LEN], date_val_str[MAX_DATE_LEN];
    int job_type_int_val, email_verified_int_val;
    const char *values_part = strchr(params, '(');

    if (!values_part) return; /* Malformed command if no ( found */

    robust_parse_insert_values(values_part, name_val, email_val, &job_type_int_val, &email_verified_int_val, date_val_str);

    new_customer.id = next_customer_id++;
    new_customer.name = strdup_custom(name_val);
    new_customer.mail = strdup_custom(email_val);
    new_customer.job_type = int_to_job_type(job_type_int_val);
    new_customer.email_verified = email_verified_int_val;
    parse_date(date_val_str, &new_customer.date_of_birth);

    add_customer_record(new_customer);
}

/* Handles the DELETE command. */
void handle_delete_command(const char *params) {
    char *where_clause;
    int delete_id;
    int i, j;

    where_clause = strstr(params, "WHERE id=");
    if (!where_clause) return; /* Malformed if no WHERE id= */

    if (sscanf(where_clause, "WHERE id=%d", &delete_id) == 1) {
        for (i = 0; i < customer_count; ) {
            if (customer_table[i].id == delete_id) {
                free_customer_fields(&customer_table[i]);
                for (j = i; j < customer_count - 1; ++j) {
                    customer_table[j] = customer_table[j+1];
                }
                customer_count--;

            } else {
                i++;
            }
        }
    }
}

/* Handles the UPDATE command. */
int handle_update_command(const char *params) {
    char set_str_full[MAX_LINE_LEN];
    char *set_clause_start, *where_clause_start;
    int update_id;
    Customer *target_customer = NULL;
    char *token_set, *saveptr_set_outer;
    char *field_name, *field_value_str, *saveptr_set_inner;
    int i;

    set_clause_start = strstr(params, " SET ");
    where_clause_start = strstr(params, " WHERE id=");

    if (!set_clause_start || !where_clause_start || set_clause_start > where_clause_start) return 1;

    /* Extract the part between " SET " and " WHERE id=" */
    strncpy(set_str_full, set_clause_start + strlen(" SET "), where_clause_start - (set_clause_start + strlen(" SET ")));
    set_str_full[where_clause_start - (set_clause_start + strlen(" SET "))] = '\0';

    if (sscanf(where_clause_start, " WHERE id=%d", &update_id) != 1) return 1;

    for (i = 0; i < customer_count; ++i) {
        if (customer_table[i].id == update_id) {
            target_customer = &customer_table[i];
            break;
        }
    }
    if (!target_customer) return 1;

    /* Parse "field=value" pairs from the SET clause */
    token_set = strtok_r(set_str_full, ",", &saveptr_set_outer);
    while(token_set) {
        char current_set_item[MAX_FIELD_LEN];
        strncpy(current_set_item, token_set, MAX_FIELD_LEN-1);
        current_set_item[MAX_FIELD_LEN-1] = '\0';

        field_name = strtok_r(current_set_item, "=", &saveptr_set_inner);
        field_value_str = strtok_r(NULL, "", &saveptr_set_inner);

        if (field_name && field_value_str) {
            trim_whitespace(field_name);
            trim_whitespace(field_value_str);
            trim_quotes_if_present(field_value_str);

            if (strcmp(field_name, "name") == 0) {
                free(target_customer->name); target_customer->name = strdup_custom(field_value_str);
            } else if (strcmp(field_name, "email") == 0 || strcmp(field_name, "mail") == 0) {
                free(target_customer->mail); target_customer->mail = strdup_custom(field_value_str);
            } else if (strcmp(field_name, "job_type") == 0) {
                target_customer->job_type = int_to_job_type(atoi(field_value_str));
            } else if (strcmp(field_name, "email_verified") == 0) {
                if (strcmp(field_value_str, "true") == 0) target_customer->email_verified = 1;
                else if (strcmp(field_value_str, "false") == 0) target_customer->email_verified = 0;
                else target_customer->email_verified = atoi(field_value_str);
            } else if (strcmp(field_name, "date") == 0) {
                parse_date(field_value_str, &target_customer->date_of_birth);
            }
        }
        token_set = strtok_r(NULL, ",", &saveptr_set_outer);
    }
    return 0;
}

/* Handles the TRUNCATE TABLE CUSTOMER command. Clears all records and resets ID counter. */
void handle_truncate_command(void) {
    int i;
    for (i = 0; i < customer_count; ++i) free_customer_fields(&customer_table[i]);

    if (customer_table != NULL) { free(customer_table); customer_table = NULL; }

    customer_count = 0;
    table_capacity = 0;
    next_customer_id = 1;
}

/* Processes commands from the command file. */
void process_commands(const char *commands_filename, const char *output_filename) {
    FILE *cmd_file = fopen(commands_filename, "r");
    char line_buffer[MAX_LINE_LEN];
    char command_type[50];
    char *params_part;
    int update_error_occurred = 0;

    if (!cmd_file) { perror("Error opening commands file"); return; }

    while (fgets(line_buffer, sizeof(line_buffer), cmd_file)) {
        char *line_ptr = line_buffer;

        line_ptr[strcspn(line_ptr, "\r\n")] = 0;
        line_ptr[strcspn(line_ptr, ";")] = 0;
        trim_whitespace(line_ptr);

        if (strlen(line_ptr) == 0) continue;

        update_error_occurred = 0;

        /* Separate command type from parameters */
        params_part = strchr(line_ptr, ' ');
        if (params_part) {
            strncpy(command_type, line_ptr, params_part - line_ptr);
            command_type[params_part - line_ptr] = '\0';
            params_part++;
            while(*params_part && isspace((unsigned char)*params_part)) params_part++;
        } else {
            strncpy(command_type, line_ptr, sizeof(command_type)-1);
            command_type[sizeof(command_type)-1] = '\0';
            params_part = "";
        }

        if (strcmp(command_type, "INSERT") == 0 && strncmp(params_part, "INTO CUSTOMER", strlen("INTO CUSTOMER")) == 0) {
            char *actual_params_for_insert = params_part + strlen("INTO CUSTOMER");
            while(*actual_params_for_insert && isspace((unsigned char)*actual_params_for_insert)) actual_params_for_insert++;
            handle_insert_command(actual_params_for_insert);
        } else if (strcmp(command_type, "DELETE") == 0 && strncmp(params_part, "FROM CUSTOMER", strlen("FROM CUSTOMER")) == 0) {
             handle_delete_command(params_part);
        } else if (strcmp(command_type, "UPDATE") == 0 && strncmp(params_part, "CUSTOMER", strlen("CUSTOMER")) == 0) {
             update_error_occurred = handle_update_command(params_part);
        } else if (strcmp(command_type, "TRUNCATE") == 0 && strncmp(params_part, "TABLE CUSTOMER", strlen("TABLE CUSTOMER")) == 0) {
            handle_truncate_command();
        }

        print_separator_and_table_to_file(output_filename, update_error_occurred, update_error_occurred ? "error" : NULL);
    }
    fclose(cmd_file);
}

void cleanup_database(void) {
    int i;
    for (i = 0; i < customer_count; ++i) free_customer_fields(&customer_table[i]);

    if (customer_table) { free(customer_table); customer_table = NULL; }

    customer_count = 0;
    table_capacity = 0;
    next_customer_id = 1;
}
