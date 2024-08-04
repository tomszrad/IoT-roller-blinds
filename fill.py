import os

def read_values_to_change(file_path):
    with open(file_path, 'r') as file:
        return [line.strip() for line in file.readlines()]

def replace_in_file(file_path, old_value, new_value):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
            content = file.read()
        
        new_content = content.replace(old_value, new_value)
        
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(new_content)
    except Exception as e:
        print(f"Error processing file {file_path}: {e}")

def main():
    values_to_change = read_values_to_change('values_to_change.txt')
    
    replacement_map = {}
    for value in values_to_change:
        new_value = input(f"What do you want to replace {value} with? ")
        if new_value:  # Check if the new value is not an empty string
            replacement_map[value] = new_value
    
    for root, dirs, files in os.walk('.'):
        for file in files:
            file_path = os.path.join(root, file)
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    if any(value in content for value in replacement_map.keys()):
                        for old_value, new_value in replacement_map.items():
                            replace_in_file(file_path, old_value, new_value)
                        print(f"Replaced values in file: {file_path}")
            except Exception as e:
                print(f"Error reading file {file_path}: {e}")

if __name__ == "__main__":
    main()
